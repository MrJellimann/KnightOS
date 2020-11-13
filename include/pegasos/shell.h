#ifndef H_PSHELL
#define H_PSHELL

#include <../sample/00-pegasos/kernel.h>
#include <circle/types.h>
#include <circle/string.h>
#include <circle/interrupt.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/usb/usbhcidevice.h>
#include <SDCard/emmc.h>
#include <fatfs/ff.h>
#include <../sample/00-pegasos/kernel.h>

#define PMAX_INPUT_LENGTH 256
#define PMAX_DIRECTORY_LENGTH 1024
#define DRIVE "SD:"

class PShell
{
    public:
        PShell(void);
        ~PShell(void);
        static void AssignKernel(CKernel* _kernel);

        static void CommandLineIn(const char* keyInput);
        static void SplitCommandLine(const char* input);
        static void CommandMatch(const char *commandName);
        static void DisplayUserWithDirectory();
        static char* GetCurrentUsername();
    
    private:
        static PShell *s_pThis;
    
    protected:
};

#endif