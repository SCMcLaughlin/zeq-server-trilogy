
#include "main_thread.h"
#include "login_crypto.h"
#include "util_socket_lib.h"
#include "zone_id.h"
#include "enum2str.h"

int main(int argc, const char** argv)
{
    MainThread* mt;
    int rc;

    (void)argc;
    (void)argv;
    
    login_crypto_init_lib();

    rc = zone_id_init();
    if (rc)
    {
        fprintf(stderr, "ERROR: zone_id_init() failed, error: %s\n", enum2str_err(rc));
        return EXIT_FAILURE;
    }

    if (!socket_lib_init())
    {
        fprintf(stderr, "ERROR: socket_lib_init() failed\n");
        return EXIT_FAILURE;
    }
    
    mt = mt_create();

    if (mt)
    {
        mt_main_loop(mt);
        mt_destroy(mt);
    }
    
    socket_lib_deinit();
    zone_id_deinit();
    login_crypto_deinit_lib();

    return EXIT_SUCCESS;
}
