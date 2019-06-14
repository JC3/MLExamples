// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// %COPYRIGHT_END%
// --------------------------------------------------------------------
// %BANNER_END%

#include <ml_lifecycle.h>
#include <ml_logging.h>
//#include <ml_perception.h>

#include "tinyxml.h"


static const char APP_TAG[] = "ManifestParsing";

int main () {

    MLLifecycleInit(NULL, NULL);

    TiXmlDocument manifest;
    if (!manifest.LoadFile("manifest.xml")) {
        ML_LOG_TAG(Error, APP_TAG, "Failed to load manifest.xml: %s", manifest.ErrorDesc());
        return 1;
    }

    ML_LOG_TAG(Info, APP_TAG, "Loaded manifest.");

    MLLifecycleSetReadyIndication();

    return 0;

}
