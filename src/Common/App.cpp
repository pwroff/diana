#include "utils/Loger.h"
#include "App.h"
#include "core/Platform.h"

namespace diana
{
    void App::Start()
    {
        auto* platform = Platform::GetInstance();
        
		try {
			platform->Initialize();
			platform->Run();
		}
		catch (std::exception &e)
		{
			LOG_S("Exception caught %s", e.what());
			MessageBox(0, e.what(), 0, 0);
		}
		catch (DxException &e)
		{
			LOG_S("DX Exception caught %ls", e.what().c_str());
			MessageBox(0, LPCSTR(e.what().c_str()), 0, 0);
		}

    }
}
