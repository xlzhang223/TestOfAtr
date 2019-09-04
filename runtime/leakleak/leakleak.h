//zhangxianlong

#ifndef _ZHANG_LEAKLEAK_H_
#define _ZHANG_LEAKLEAK_H_
#include <fstream>
#include <memory>
#include <string>
namespace art {
    namespace mirror {
        class Object;
    }

    namespace leakleak{

        // void dump_info();
        void dump_init();
        void dump_str(std::string S);
        // void dump_obj(mirror::Object *obj, std::string s = ".");
        // void pushback(mirror::Object *obj);
        // void dump_vct();
        void addedge(mirror::Object *obj,mirror::Object *ref);
        void check_file();
        void gc_begin();
        void gc_end();
    }
}
#endif
//end
//  Thread* self = Thread::Current();
//  Runtime::Current()->GetClassLinker()->FindClass(self, "android/app/activity",
//                                                       ScopedNullHandle<mirror::ClassLoader>()));