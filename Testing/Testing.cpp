#include <iostream>
#include "math/mat4.h"
#include "misc/string_helpers.h"
#include "io/mesh_io.h"
#include "io/game_clock.h"
#include "io/console.h"

namespace
{
    void matrixTesting()
    {
        auto a = ema::mat4::Identity();
        auto b = ema::mat4::Translation({ 1.0f, 0.0f, 0.0f });
        auto c = a * b;
        std::cout << c;

        ema::vec4 v(0.0f, 0.0f, 0.0f, 1.0f);
        auto v2 = v * c;
        std::cout << v2.x << " " << v2.y << " " << v2.z << " " << v2.w << std::endl;
    }
}

int main()
{
    //matrixTesting();
    eio::GameClock clock;
    eio::Console::InitConsole2(&clock);

    eio::ConvertOBJToOBJB("../Anti-Aliasing/models/sponza");
    eio::ConvertOBJToOBJB("../Anti-Aliasing/models/knight");
    eio::ConvertOBJToOBJB("../Anti-Aliasing/models/good-well");
}
