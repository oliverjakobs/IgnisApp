#include "examples/examples.h"

int main()
{
    //MinimalApp app = example_cube();
    MinimalApp app = example_gjk();

    if (minimalLoad(&app, "IgnisApp", 1024, 800, "4.4"))
        minimalRun(&app);

    minimalDestroy(&app);

    return 0;
}