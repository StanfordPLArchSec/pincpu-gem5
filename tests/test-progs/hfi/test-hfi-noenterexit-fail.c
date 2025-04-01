#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "hfi.h"

int main(int argc, char* argv[])
{
    // exit without enter sandbox should fail
    hfi_exit_sandbox();
}
