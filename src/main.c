
#include "main_thread.h"
#include "login_crypto.h"

int main(int argc, const char** argv)
{
    MainThread* mt;

    (void)argc;
    (void)argv;
    
    login_crypto_init_lib();
    
    mt = mt_create();

    if (mt)
    {
        mt_main_loop(mt);
        mt_destroy(mt);
    }
    
    login_crypto_deinit_lib();

    return EXIT_SUCCESS;
}
