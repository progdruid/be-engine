// ReSharper disable CppInconsistentNaming

#pragma once

#ifndef BE_ACCESS_MODIFIERS
    #define BE_ACCESS_MODIFIERS

    #define pub     private:public:     [[]]
    #define pro     private:protected:  [[]]
    #define pri     public: private:    [[]]
#endif
