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
namespace art {
    namespace mirror {
        class Object;
        //class ObjPtr;
    }

    namespace leakleak{
        const int len = 256;
        class myio{
            public:
            std::vector<int> get_int();
            std::vector<std::string> get_str();
            std::string get_app();
            void put_ans(std::string ans);
        };
        const int LEN = 128;
        class Leaktrace{
            private:
            myio io;
            int read_time;
            int map_time;
            int gc_time;
            int name_time;
            int code_time;
            int objcnt;
            bool istrace;
            //int fp_int;
            //int fp_str;
            int GC_K;
            uint64_t main_begin;
            uint64_t main_end;
            uint64_t heap_end;
            std::unordered_map<uint64_t,uint64_t> addr_pc,addr_pc_t;
            std::unordered_set<uint64_t> s_ans;
            std::unordered_set<uint64_t> s_obj;
            public:
            Leaktrace(){
                read_time = 0 ;
                map_time = 0 ;
                gc_time = 0 ;
                name_time = 0;
                code_time = 0 ;
                objcnt = 0;
                istrace = false;
                //fp_int=0;
                //fp_str=0;
                GC_K=0;
                main_begin = 0;
                main_end = 0;
                heap_end = 0;
            };
            static Leaktrace& getInstance(){
                static Leaktrace instance;
                return instance;
            };
            void set_main_begin(uint64_t _begin);
            void set_main_end(uint64_t _end);
            void set_heap_end(uint64_t _end);
            uint64_t get_main_begin();
            uint64_t get_main_end();
            uint64_t get_heap_end();
            void dump_info();

            
            //static bool GetProcNameByPid(char *buf, size_t size, int pid);
            static bool GetProcNameByPid(char *buf, size_t size, int pid) {
                //int t = clock();
                snprintf(buf, size, "/proc/%d/cmdline", pid);
                FILE *fp = fopen(buf, "r");
                if (fp) {
                    char tmp[LEN];
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

            void addedge(mirror::Object *obj)REQUIRES_SHARED(Locks::mutator_lock_);

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
            void heap_init()REQUIRES_SHARED(Locks::mutator_lock_);

            bool get_istrace();

            void log_app_name();

            void gc_begin();

            void gc_end()REQUIRES_SHARED(Locks::mutator_lock_);
            void dump_str_ui32(mirror::Object *I)REQUIRES_SHARED(Locks::mutator_lock_);

            void dump_str_i_i(mirror::Object *I,mirror::Object *J)REQUIRES_SHARED(Locks::mutator_lock_);

            void new_obj(mirror::Object *obj)REQUIRES_SHARED(Locks::mutator_lock_);
        };

    }
}
#endif
//end
//  Thread* self = Thread::Current();
//  Runtime::Current()->GetClassLinker()->FindClass(self, "android/app/activity",
//                                                       ScopedNullHandle<mirror::ClassLoader>()));