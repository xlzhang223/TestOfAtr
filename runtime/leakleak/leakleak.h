//zhangxianlong

#ifndef _ZHANG_LEAKLEAK_H_
#define _ZHANG_LEAKLEAK_H_
#include <fstream>
#include <memory>
#include <string>
#include <utils/Log.h>
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
#include "safe_map.h"
#include "jit/jit.h"
#include "jit/jit_code_cache.h"
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
        // static char tmp[LEN];
        // static char name[LEN];
        class Leaktrace{
            private:
            myio io;
            // int gc_time;
            int objcnt;
            int t_size;
            bool istrace;
            bool jitinfo;
            int GC_K;
            std::set<mirror::Class *> classes_;
            uint64_t main_begin;
            uint64_t main_end;
            uint64_t bitmap_ptr;
            std::unordered_map<uint32_t,std::pair<uintptr_t,uint16_t>> addr_pc;
            std::unordered_map<uint32_t,int> vis;
            std::unordered_map<uintptr_t,int> m_ans;
            uint32_t low(mirror::Object * o){return (uint32_t)(reinterpret_cast<uint64_t>((void*)o)-main_begin);};
            void NewClass(mirror::Class *klass);
            void NewMethodLinked(const std::string& class_name, ArtMethod &method);
            int try_thread_istrace();
            public:
            // std::fstream jit_info;
            Leaktrace(){
                // gc_time = 0 ;
                objcnt = 0;
                istrace = false;
                GC_K=0;
                main_begin = 0;
                main_end = 0;
                bitmap_ptr = 0;
                t_size = 48;
                jitinfo = false;
            };
            bool need_jitinfo(){return jitinfo;};
            void new_method(ArtMethod * method);
            void put_jit(uint8_t* code,const std::string method_name,bool osr);
            int getTsize(){return t_size;};
            static Leaktrace& getInstance(){
                static Leaktrace instance;
                return instance;
            };
            void set_main_begin(uint64_t _begin);
            void set_main_end(uint64_t _end);
            void set_bitmap_ptr(uint64_t _end);
            uint64_t get_main_begin();
            uint64_t get_main_end();
            uint64_t get_bitmap_ptr();
            void dump_info(uint64_t tim);

            
            //static bool GetProcNameByPid(char *buf, size_t size, int pid);
            static bool GetProcNameByPid(char *buf, size_t size, int pid) {
                //int t = clock();
                snprintf(buf, size, "/proc/%d/cmdline", pid);
                FILE *fp = fopen(buf, "r");
                if (fp) {
                    char tmp[len];
                    if (fread(tmp, sizeof(char), size, fp))
                        strcpy(buf, tmp);
                    fclose(fp);
                    //t = clock() - t;
                    //read_time += t; 
                    return true;
                }
                return false;
            }


            void check_file();

            //do nothing 
            void dump_vct();

            void addedge(mirror::Object *obj);

            void dump_str(std::string S);

            void dump_init();
            
            // void dump_obj(mirror::Object *obj, std::string s){
            //     //  char name[LEN] = "noname";
                
            //     // GetProcNameByPid(name, LEN, getpid());
            //     // if(strstr(name,"myapp")==NULL) return;
            //     // ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
            //     // std::string sname=obj->PrettyTypeOf();
                
            //     // ALOGD(" ZHANG,GC-OBJ %s,HASHCODE: %d ,INFO: %s!",sname.c_str(),obj->IdentityHashCode(),s.c_str());
            // }
            void heap_init();

            bool get_istrace();

            void log_app_name();

            void gc_begin();

            void gc_end();
            void interpreter_touch_obj(mirror::Object *I);

            void CC_move_from_to(mirror::Object *I,mirror::Object *J);

            void new_obj(mirror::Object *obj,int _byte);
        };

    }
}
#endif
//end
//  Thread* self = Thread::Current();
//  Runtime::Current()->GetClassLinker()->FindClass(self, "android/app/activity",
//                                                       ScopedNullHandle<mirror::ClassLoader>()));