
#include "main_thread.h"
#include "login_crypto.h"
#include "util_socket_lib.h"

int main(int argc, const char** argv)
{
    MainThread* mt;

    (void)argc;
    (void)argv;
    
    login_crypto_init_lib();
    
    if (socket_lib_init())
    {
        mt = mt_create();

        if (mt)
        {
            mt_main_loop(mt);
            mt_destroy(mt);
        }
    }
    
    socket_lib_deinit();
    login_crypto_deinit_lib();

    return EXIT_SUCCESS;
}
