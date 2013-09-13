#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_lua_init.h"
#include "ngx_lua_content.h"
#include "ngx_lua_code.h"

#include "ngx_lua_module.h"

// http module
void* ngx_lua_create_main_conf(ngx_conf_t* cf);
void* ngx_lua_create_loc_conf(ngx_conf_t* cf);
char* ngx_lua_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child);

// module
ngx_int_t ngx_lua_init_process(ngx_cycle_t* cycle);
void ngx_lua_exit_process(ngx_cycle_t* cycle);

ngx_command_t ngx_lua_commands[] = {
    {
        ngx_string("lua_init"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_lua_init_readconf,
        NGX_HTTP_MAIN_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("lua_init_by_file"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_lua_init_readconf,
        NGX_HTTP_MAIN_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("lua_content"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_lua_content_readconf,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_lua_loc_conf_t, lua_content_code),
        NULL
    },
    {
        ngx_string("lua_content_by_file"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_lua_content_readconf,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_lua_loc_conf_t, lua_content_code),
        NULL
    },
    {
        ngx_string("lua_code_cache"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_FLAG,
        ngx_lua_code_cache_readconf,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_lua_loc_conf_t, enable_code_cache),
        NULL
    },
    ngx_null_command
};

ngx_http_module_t ngx_lua_module_ctx = {
    NULL,                     // pre configuration
    NULL,                     // post configuration
    ngx_lua_create_main_conf, // create main conf
    NULL,                     // init main conf
    NULL,                     // create srv conf
    NULL,                     // merge srv conf
    ngx_lua_create_loc_conf,  // create loc conf
    ngx_lua_merge_loc_conf    // merge loc conf
};

ngx_module_t ngx_lua_module = {
    NGX_MODULE_V1,
    &ngx_lua_module_ctx,
    ngx_lua_commands,
    NGX_HTTP_MODULE,
    NULL,                 // init master
    NULL,                 // init module
    ngx_lua_init_process, // init process
    NULL,                 // init thread
    NULL,                 // exit thread
    ngx_lua_exit_process, // exit process
    NULL,                 // exit master
    NGX_MODULE_V1_PADDING
};

void* ngx_lua_create_main_conf(ngx_conf_t* cf)
{
    ngx_lua_main_conf_t* pconf;
    printf("ngx_lua_create_main_conf\n");

    pconf = ngx_pcalloc(cf->pool, sizeof(ngx_lua_main_conf_t));
    if (pconf == NULL) return NGX_CONF_ERROR;

    return pconf;
}

void* ngx_lua_create_loc_conf(ngx_conf_t* cf)
{
    ngx_lua_loc_conf_t* conf;
    printf("ngx_lua_create_loc_conf\n");
    
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_lua_loc_conf_t));
    if (conf == NULL) return NGX_CONF_ERROR;

    ngx_str_null(&conf->lua_content_code);
    return conf;
}

char* ngx_lua_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child)
{
    ngx_lua_loc_conf_t* prev = parent;
    ngx_lua_loc_conf_t* conf = child;
    printf("ngx_lua_merge_loc_conf\n");

    /*if (conf->lua_content_code.len == 0)
    {
        conf->lua_content_code = prev->lua_content_code;
    }*/

    ngx_conf_merge_value(conf->enable_code_cache, prev->enable_code_cache, 1);

    return NGX_CONF_OK;
}

ngx_int_t ngx_lua_init_process(ngx_cycle_t* cycle)
{
    ngx_lua_main_conf_t* pconf;
    printf("ngx_lua_init_process\n");

    pconf = ngx_http_cycle_get_module_main_conf(cycle, ngx_lua_module);
    pconf->lua = luaL_newstate();

    luaL_openlibs(pconf->lua);

    if (pconf->lua_init_code.len)
    {
        if (luaL_loadbuffer(pconf->lua, (const char*)pconf->lua_init_code.data, pconf->lua_init_code.len, "@lua_init"))
        {
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: luaL_loadbuffer error");
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: %s", lua_tostring(pconf->lua, -1));
            return NGX_ERROR;
        }
        if (lua_pcall(pconf->lua, 0, 0, 0))
        {
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: lua_pcall error");
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: %s", lua_tostring(pconf->lua, -1));
            return NGX_ERROR;
        }
    }
    return NGX_OK;
}

void ngx_lua_exit_process(ngx_cycle_t* cycle)
{
    ngx_lua_main_conf_t* pconf;
    printf("ngx_lua_exit_process\n");

    pconf = ngx_http_cycle_get_module_main_conf(cycle, ngx_lua_module);
    if (pconf->lua) lua_close(pconf->lua);
}