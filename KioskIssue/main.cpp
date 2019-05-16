#include <ml_lifecycle.h>
#include <ml_privileges.h>
#include <ml_logging.h>

static const char APP_TAG[] = "KioskIssue";


int main() {

    ML_LOG_TAG(Info, APP_TAG, "Running...");

    MLLifecycleInit(NULL, NULL);
    MLPrivilegesStartup();

    ML_LOG_TAG(Info, APP_TAG, "Requesting CameraCapture privilege...");
    
    if (MLPrivilegesRequestPrivilege(MLPrivilegeID_CameraCapture) == MLPrivilegesResult_Granted)
        ML_LOG_TAG(Info, APP_TAG, "CameraCapture privilege granted.");
    else
        ML_LOG_TAG(Error, APP_TAG, "CameraCapture privilege denied.");

    MLLifecycleSetReadyIndication();

    ML_LOG_TAG(Info, APP_TAG, "Finished.");

}
