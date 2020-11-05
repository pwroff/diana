#pragma once
#include <memory>
#include <typeinfo>
#include "shared.h"

namespace diana
{
    template<class T>
    class Singletone
    {
    private:
        static std::unique_ptr<T> Instance;

    public:
        
        Singletone<T>()
        {
            LOG_S("Creating %s instance", typeid(T).name());
        }
        ~Singletone<T>()
        {
            LOG_S("instance %s released", typeid(T).name());
        }

        static T* GetInstance()
        {
			if (Instance != nullptr)
				return Instance.get();
			Instance.reset(new T);
			return Instance.get();
        }
    };

    template<class T>
    std::unique_ptr<T> Singletone<T>::Instance = {nullptr};
}
