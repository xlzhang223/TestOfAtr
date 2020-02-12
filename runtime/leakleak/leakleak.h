//zhangxianlong

#ifndef _ZHANG_LEAKLEAK_H_
#define _ZHANG_LEAKLEAK_H_
#include <fstream>
#include <memory>
#include <string>
#include <log/log.h>
#include <jni.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sys/msg.h>
// #include "safe_map.h"
#include "jit/jit.h"
#include "jit/jit_code_cache.h"
#include "gc/collector/gc_type.h"
namespace art {

    class ArtMethod;
    namespace mirror {
        class Object;
        class Class;
        //class ObjPtr;
    }
    namespace jit{
        class Jit;
        class JitCodeCache;
    }

    namespace leakleak{
        const int len = 127;
        class myio{
            public:
            std::vector<int> get_int(std::string path);
            std::vector<std::string> get_str();
            std::vector<std::string> get_app();
            void put_ans(std::string ans);
        };
        const int LEN = 127;

        class Leaktrace{
            private:
            myio io;
            int objcnt;
            int t_size;
            bool istrace;
            bool jitinfo;
            int GC_K;
            uint32_t large_size;
            std::set<std::pair<mirror::Class *,void*>> classes_;
            uint64_t main_begin;
            uint64_t main_end;
            uint64_t bitmap_ptr;
            std::unordered_map<uint32_t,std::pair<uintptr_t,uint16_t>> addr_pc;
            std::unordered_map<uint32_t,std::pair<uintptr_t,uint16_t>> addr_large_pc;
            std::unordered_map<uintptr_t,int> m_ans;
            uint32_t low(mirror::Object * o){return (uint32_t)(reinterpret_cast<uint64_t>((void*)o)-main_begin);};
            mirror::Object * high(uint32_t o){return reinterpret_cast<mirror::Object *>((void*)(o+main_begin));};
            void NewClass(mirror::Class *klass,void* pc);
            void NewMethodLinked(const std::string& class_name, ArtMethod &method);
            int try_thread_istrace();
            Leaktrace(const Leaktrace&) = delete;
            Leaktrace& operator=(const Leaktrace&) = delete;
            public:
            void heap_end();
            Leaktrace(){
                // LOG(WARNING)<<"Leakleak,Leaktrace init!";
                objcnt = 0;
                istrace = 0;
                GC_K=0;
                main_begin = 0;
                main_end = 0;
                bitmap_ptr = 0;
                t_size = 48;
                large_size = 12 * 1024;
                jitinfo = false;
            };
        
            bool need_jitinfo(){return jitinfo;};

            void new_method(ArtMethod * method);

            void put_jit(uint8_t* code,const std::string method_name,bool osr);

            int getTsize(){return t_size;};

            void set_main_begin(uint64_t _begin);

            void set_main_end(uint64_t _end);

            void set_large_size(uint32_t large_size_){
                large_size = large_size_;
            }

            void set_bitmap_ptr(uint64_t _end);

            uint64_t get_main_begin();

            uint64_t get_main_end();

            uint64_t get_bitmap_ptr();

            void dump_info(uint64_t tim);

            static bool GetProcNameByPid(char *buf, size_t size, int pid) {
                snprintf(buf, size, "/proc/%d/cmdline", pid);
                FILE *fp = fopen(buf, "r");
                if (fp) {
                    char tmp[len];
                    if (fread(tmp, sizeof(char), size, fp))
                        strcpy(buf, tmp);
                    fclose(fp);
                    return true;
                }
                return false;
            }


            void check_file();

            void dump_vct();

            void addedge(mirror::Object *obj,gc::collector::GcType ty);

            void dump_str(std::string S);

            void dump_init();
            
            void heap_init();

            bool get_istrace();

            void log_app_name();

            void gc_begin(gc::collector::GcType ty);

            void gc_end(gc::collector::GcType ty);

            void interpreter_touch_obj(mirror::Object *I);

            void CC_move_from_to(mirror::Object *I,mirror::Object *J);

            void new_obj(mirror::Object *obj,uint32_t _byte);

            void Free_obj(mirror::Object *obj);

        };
        Leaktrace* getInstance();
    }
}
#endif
//end
//  Thread* self = Thread::Current();
//  Runtime::Current()->GetClassLinker()->FindClass(self, "android/app/activity",
//                                                       ScopedNullHandle<mirror::ClassLoader>()));