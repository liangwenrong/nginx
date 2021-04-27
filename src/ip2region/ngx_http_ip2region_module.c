#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_ip2region.h"

/**
 * 存储字段数组下标
 */
#define IP2REGION_CITY_ID 0
#define IP2REGION_REGION_NAME 1
#define IP2REGION_COUNTRY_NAME 2
#define IP2REGION_PROVINCE_NAME 3
#define IP2REGION_CITY_NAME 4
#define IP2REGION_NET_TYPE 5
#define IP2REGION_ISP_DOMAIN 6

/**
 * 初始化函数声明
 */
static void*
ngx_http_ip2region_create_main_conf(ngx_conf_t *cf);
static char*
ngx_http_ip2region_init_main_conf(ngx_conf_t *cf, void *conf);

static ngx_int_t
ngx_http_ip2region_init_process(ngx_cycle_t *cycle);
static void
ngx_http_ip2region_exit_process(ngx_cycle_t *cycle);

/**
 * 函数声明
 */
static ngx_int_t
ngx_http_variable_get_handler_str(ngx_http_request_t *r, ngx_http_variable_value_t *v, u_char *data, ngx_int_t len);
static u_char *
ngx_http_ip2region_get_variable(ngx_http_request_t *r, u_char *buf, int IP2REGION_INDEX);
static u_char *
ngx_http_ip2region_get_region_name(ngx_http_request_t *r, u_char *buf);
/**
 * 要添加的变量声明
 */
static ngx_int_t ngx_http_ip2region_add_variable(ngx_conf_t *cf);

static ngx_int_t ngx_http_variable_ip2region_city_id(ngx_http_request_t *r,
                                        ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_region_name(ngx_http_request_t *r,
                                        ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_country_name(ngx_http_request_t *r,
                                         ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_province_name(ngx_http_request_t *r,
                                                            ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_city_name(ngx_http_request_t *r,
                                                            ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_net_type(ngx_http_request_t *r,
                                                            ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_isp_domain(ngx_http_request_t *r,
                                                            ngx_http_variable_value_t *v, uintptr_t data);


/**
 * 需要添加的变量
 * 数组
 */
static ngx_http_variable_t ngx_http_ip2region_variables[] = {
        {
                ngx_string("ip2region_city_id"),
                NULL, ngx_http_variable_ip2region_city_id,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_region_name"),
                NULL, ngx_http_variable_ip2region_region_name,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_country_name"),
                NULL, ngx_http_variable_ip2region_country_name,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_province_name"),
                NULL, ngx_http_variable_ip2region_province_name,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_city_name"),
                NULL, ngx_http_variable_ip2region_city_name,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_net_type"),
                NULL, ngx_http_variable_ip2region_net_type,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_isp_domain"),
                NULL, ngx_http_variable_ip2region_isp_domain,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


/**
 * 读取 nginx 配置
 */
static ngx_conf_enum_t ngx_ip2region_search_algo[] = {
        {ngx_string("memory"), NGX_IP2REGION_MEMORY},
        {ngx_string("binary"), NGX_IP2REGION_BINARY},
        {ngx_string("btree"),  NGX_IP2REGION_BTREE},
        {ngx_null_string, NGX_IP2REGION_MEMORY}//默认
};
static ngx_command_t  ngx_http_ip2region_commands[] = {
        {
                ngx_string("ip2region"),
                NGX_MAIN_CONF | NGX_DIRECT_CONF | NGX_CONF_TAKE1,
                ngx_conf_set_flag_slot,
                0,
                offsetof(ngx_http_ip2region_conf_t, enable),
                NULL
        },
        {
                ngx_string("ip2region_db_file"),
                NGX_MAIN_CONF | NGX_DIRECT_CONF | NGX_CONF_TAKE1,
                ngx_conf_set_str_slot,
                0,
                offsetof(ngx_http_ip2region_conf_t, db_file),
                NULL
        },
        {
                ngx_string("ip2region_algo"),
                NGX_MAIN_CONF | NGX_DIRECT_CONF | NGX_CONF_TAKE1,
                ngx_conf_set_enum_slot,
                0,
                offsetof(ngx_http_ip2region_conf_t, algo),
                &ngx_ip2region_search_algo
        },

        ngx_null_command
};


/**
 * 调用添加变量函数入口，创建配置入口，初始化配置函数入口
 */
static ngx_http_module_t ngx_http_ip2region_module_ctx = {
        ngx_http_ip2region_add_variable,       /* preconfiguration */
        NULL,                                  /* postconfiguration */

        ngx_http_ip2region_create_main_conf,   /* create main configuration */
        ngx_http_ip2region_init_main_conf,     /* init main configuration */

        NULL,                                  /* create server configuration */
        NULL,                                  /* merge server configuration */
        NULL,                                  /* create location configuration */
        NULL                                   /* merge location configuration */
};
ngx_module_t  ngx_http_ip2region_module = {
        NGX_MODULE_V1,
        &ngx_http_ip2region_module_ctx,       /* module context */
        ngx_http_ip2region_commands,          /* module directives */
        NGX_HTTP_MODULE,                      /* module type */
        NULL,                                 /* init master */
        NULL,                                 /* init module */
        ngx_http_ip2region_init_process,      /* init process */
        NULL,                                 /* init thread */
        NULL,                                 /* exit thread */
        ngx_http_ip2region_exit_process,      /* exit process */
        NULL,                                 /* exit master */
        NGX_MODULE_V1_PADDING
};

static void*
ngx_http_ip2region_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_ip2region_conf_t  *ip2region_conf;
    ip2region_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_ip2region_conf_t));

    if(ip2region_conf == NULL) {
        return NULL;
    }
    ip2region_conf->enable = NGX_CONF_UNSET;
    ip2region_conf->algo = NGX_CONF_UNSET_UINT;
    return ip2region_conf;
}

static char*
ngx_http_ip2region_init_main_conf(ngx_conf_t *cf, void *conf)
{
    ngx_http_ip2region_conf_t  *ip2region_conf = (ngx_http_ip2region_conf_t *)conf;

    //todo 来到这里应该是已经初始化值了

    if (ip2region_conf->enable == NGX_CONF_UNSET || ip2region_conf->enable == 0) {
        //ngx_str_set(&ip2region_conf->db_file, "conf/ip2region.db");
        return NGX_CONF_OK;//不开启
    }

    if(ip2region_conf->db_file.len == 0) {
        //ngx_str_set(&ip2region_conf->db_file, "conf/ip2region.db");
        return NGX_CONF_ERROR;//文件路径没传
    }

    if (ngx_conf_full_name(cf->cycle, &ip2region_conf->db_file, 0) != NGX_OK) {//todo 不知道干嘛的
        return NGX_CONF_ERROR;
    }

    ngx_log_debug1(NGX_LOG_INFO, ngx_cycle->log, 0, "%V", &ip2region_conf->db_file);

    if(ip2region_conf->algo == NGX_CONF_UNSET_UINT) {
        ip2region_conf->algo = NGX_IP2REGION_MEMORY;//默认
    }

    return NGX_CONF_OK;
}


/**
 *
 */
static ngx_int_t
ngx_http_ip2region_init_process(ngx_cycle_t *cycle)
{
    ngx_http_ip2region_conf_t *conf = (ngx_http_ip2region_conf_t *)
            ngx_get_conf(ngx_cycle->conf_ctx, ngx_http_ip2region_module);

    if (conf->enable == 0) {//off
        ngx_log_debug0(NGX_LOG_INFO, ngx_cycle->log, 0, "ip2region not enable");
        return NGX_OK;
    }
    if(conf->db_file.len == 0) {
        ngx_log_debug0(NGX_LOG_ERR, ngx_cycle->log, 0, "ip2region not init,db_file not found");
        return NGX_ERROR;
    }

    return create_ip2region(conf, ngx_cycle->log);
}

static void
ngx_http_ip2region_exit_process(ngx_cycle_t *cycle)
{
    destroy_ip2region(ngx_cycle->log);
}

/**
 * 待删除函数
 */
/*
static ngx_int_t
ngx_http_datetime_fmt(ngx_http_request_t *r,
                      ngx_http_variable_value_t *v, ngx_int_t t, ngx_int_t len)
{
    u_char *p;

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%02i", t) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}
*/




/**
 * ***********************************************************
 * 变量的 gethandler 函数体
 */

/**
 * todo 这个方法不声明，则可能报错，现在是放调用者前面了
 * 统一返回str值
 */
static ngx_int_t
ngx_http_variable_get_handler_str(ngx_http_request_t *r, ngx_http_variable_value_t *v, u_char *data, ngx_int_t len)
{
    //分配内存
    u_char *p;
    p = ngx_pnalloc(r->pool, len);//todo 分配足够放下data的长度
    if (p == NULL) {
        return NGX_ERROR;
    }

    //todo 不能直接指向data地址，局部变量问题，应该复制到p
/*    //直接分配内存
    v->len = f->script_name.len + flcf->index.len;
    v->data = ngx_pnalloc(r->pool, v->len);
    if (v->data == NULL) {
        return NGX_ERROR;
    }
    //为什么要复制？
    char* pp = ngx_cpymem(v->data, f->script_name.data, f->script_name.len);
    ngx_memcpy(pp, flcf->index.data, flcf->index.len);*/

    v->len = len;//todo 应该是实际长度，不是分配长度，这里len不应该传，而是计算str的长度？
    v->valid = 1;//有了
    v->no_cacheable = 0;//可以缓存
    v->not_found = 0;//如果不设置这个变量才设为1
    v->data = p;

    return NGX_OK;
}


/**
 * 根据数组下标查找变量值
 * @param r 
 * @param buf 
 * @param IP2REGION_INDEX ngx_http_ip2region_variables 下标
 * @return 
 */
static u_char *
ngx_http_ip2region_get_variable(ngx_http_request_t *r, u_char *buf, int IP2REGION_INDEX)
{
    ngx_http_variable_value_t  *value;
    value = ngx_http_get_indexed_variable(r, ngx_http_ip2region_variables[IP2REGION_INDEX].index);

    if (value == NULL || value->not_found) {
        *buf = '-';//todo 看不懂
        return buf + 1;
    }
//    return (((u_char *) memcpy(buf, value->data, value->len)) + (value->len));
    return ngx_cpymem(buf, value->data, value->len);
}

static u_char *
ngx_http_ip2region_get_region_name(ngx_http_request_t *r, u_char *buf){
    return ngx_http_ip2region_get_variable(r, buf, ngx_http_ip2region_variables[IP2REGION_REGION_NAME].index);
}


static ngx_int_t
ngx_http_variable_ip2region_city_id(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
//    u_char *buf = NULL;//todo NULL有问题
//    u_char *region_name = ngx_http_ip2region_get_region_name(r, buf);
    
    u_char *city_id = "ngx_http_variable_ip2region_city_id";
    return ngx_http_variable_get_handler_str(r, v, city_id, strlen(city_id));//todo 长度看下是多少
}
static ngx_int_t
ngx_http_variable_ip2region_region_name(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char *XXXX = "ngx_http_variable_ip2region_region_name";
    return ngx_http_variable_get_handler_str(r, v, XXXX, strlen(XXXX));//todo 长度看下是多少
}
static ngx_int_t
ngx_http_variable_ip2region_country_name(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char *XXXX = "ngx_http_variable_ip2region_country_name";
    return ngx_http_variable_get_handler_str(r, v, XXXX, strlen(XXXX));//todo 长度看下是多少
}
static ngx_int_t
ngx_http_variable_ip2region_province_name(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char *XXXX = "ngx_http_variable_ip2region_province_name";
    return ngx_http_variable_get_handler_str(r, v, XXXX, strlen(XXXX));//todo 长度看下是多少
}
static ngx_int_t
ngx_http_variable_ip2region_city_name(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char *XXXX = "ngx_http_variable_ip2region_city_name";
    return ngx_http_variable_get_handler_str(r, v, XXXX, strlen(XXXX));//todo 长度看下是多少
}
static ngx_int_t
ngx_http_variable_ip2region_net_type(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char *XXXX = "ngx_http_variable_ip2region_net_type";
    return ngx_http_variable_get_handler_str(r, v, XXXX, strlen(XXXX));//todo 长度看下是多少
}
static ngx_int_t
ngx_http_variable_ip2region_isp_domain(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char *XXXX = "ngx_http_variable_ip2region_isp_domain";
    return ngx_http_variable_get_handler_str(r, v, XXXX, strlen(XXXX));//todo 长度看下是多少
}


/**
 * 完成
 * 遍历ngx_http_ip2region_vars数组，逐个添加变量
 * 变量的值取决于get_handler 方法
 */
static ngx_int_t
ngx_http_ip2region_add_variable(ngx_conf_t *cf)
{
    //todo 这里应该控制是否启用ip2region

    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_ip2region_variables; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;

        /**
         * 直接索引起来，方便后续获取
         */
        ngx_int_t index = ngx_http_get_variable_index(cf, &v->name);
        if (index == NGX_ERROR) {
            return NGX_ERROR;
        }
        v->index = index;//todo ngx_int_t赋值给ngx_uint_t，可能报错
    }

    return NGX_OK;
}


/**
* 借鉴
*/
//从请求的内存池中分配内存，len长度，一般用作返回数据，避免局部丢失
//ngx_http_request_t *r;
//u_char *p;
//p = ngx_pnalloc(r->pool, len);

