
#define LOG_TAG "leakleak"
#include "leakleak.h"
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
#include <fstream>
#include <sys/msg.h>

#include "gc/heap.h"
#include "runtime.h"
#include "gc/space/large_object_space.h"
#include "utils.h"
#include "class_linker.h"
#include "art_method-inl.h"
#include "mirror/object_array-inl.h"
#include "mirror/class-inl.h"
#include "thread.h"
#include "scoped_thread_state_change-inl.h"

namespace art{

    namespace mirror {
        class Object;
    }


    namespace leakleak{

        // class msg{
        //     struct msg_st{
        //         long int msg_type;
        //         char text[128];
        //     };
        //     struct msg_st data;
        //     int running = 1;
        //     int msgid = -1;
        //     long int msgtype = 0; //注意1

        //     public: std::vector<int> getint(){
        //         std::vector<int> ret;
        //         msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
        //         if(msgid == -1){
        //             return ret;
        //         }
        //         //从队列中获取消息，直到遇到end消息为止
        //         while(running){
        //             int f = msgrcv(msgid, (void*)&data, 128, msgtype, IPC_NOWAIT);
        //             if( f == -1 || f == ENOMSG ){
        //                 break;
        //             }
        //             // std::cout<<"rev:"<<data.text<<"\n"; 
        //             std::stringstream ss;
        //             ss<<data.text;
        //             int x=0;
        //             ss>>x;
        //             ret.push_back(x);
        //         }
        //         return ret;
        //     }
        // };

        const int LEN = 128 ;
        int read_time = 0 ;
        int map_time = 0 ;
        int gc_time = 0 ;
        int name_time = 0;
        int code_time = 0 ;
        int objcnt = 0;
        bool istrace = false;
        int fp_int=0;
        int fp_str=0;

        class myio{
        const int len = 256;
        public:
        std::vector<int> get_int(){
            std::vector<int> v;
            int ret = -1;
            char tmp[len];
            FILE *fp = fopen("/storage/self/primary/Download/hash.txt","r");
            if(fp){
                //fseek(fp,fp_int*64,SEEK_SET);
                memset(tmp,0,sizeof(tmp));
                while(fread(tmp,sizeof(char),64,fp)){
                    std::stringstream ss(tmp);
                    ss>>ret;
                    v.push_back(ret);
                    memset(tmp,0,sizeof(tmp));
                    fp_int++;
                };
                fclose(fp);
                // fp = fopen("/storage/self/primary/Download/hash.txt","w");
                // if(fp) fclose(fp);
                
            }
            return v;
        }
        std::vector<std::string> get_str(){
            std::vector<std::string> v;
            std::string ret = "-1";
            char tmp[len];
            FILE *fp = fopen("/storage/self/primary/Download/name.txt","r");
            if(fp){
                //fseek(fp,fp_str*64,SEEK_SET);
                memset(tmp,0,sizeof(tmp));
                while(fread(tmp,sizeof(char),64,fp)){
                    std::stringstream ss(tmp);
                    ss>>ret;
                    v.push_back(ret);
                    memset(tmp,0,sizeof(tmp));
                    fp_str++;
                };
                fclose(fp);
                // fp = fopen("/storage/self/primary/Download/name.txt","w");
                // if(fp) fclose(fp);
            }
            return v;

        }
        std::string get_app(){
            // std::vector<std::string> v;
            std::string ret = "-1";
            char tmp[len];
            FILE *fp = fopen("/storage/self/primary/Download/app.txt","r");
            if(fp){
                memset(tmp,0,sizeof(tmp));
                while(fread(tmp,sizeof(char),len,fp)){
                    std::stringstream ss(tmp);
                    // while(ss>>ret)v.push_back(ret);
                    ss>>ret;
                };
                fclose(fp);
                // fp = fopen("/storage/self/primary/Download/name.txt","w");
                // if(fp) fclose(fp);
            }
            return ret;
        }

    }io;
    
    class Finder{
        private:
        std::unordered_map<mirror::Object*,std::vector<mirror::Object*>> father;
        // std::unordered_map<int,mirror::Object*> map_;
        std::unordered_set<int> _set;
        std::unordered_multiset<std::string> _set2;

        std::stack<mirror::Object*> tmp;
        std::unordered_set<mirror::Object*> vis;
        int flag=0;

        void dfs(mirror::Object* now,int dp,int deep){
            if(dp>deep||flag==1) return;
            tmp.push(now);
            vis.insert(now);
            if(father[now].size()==0||dp >= 10){
                flag=1;
                vis.erase(now);
                return;
            }
            for(auto next:father[now]){
                if(vis.count(next)==0)
                    dfs(next,dp+1,deep);
            }
            vis.erase(now);
            if(flag==1) return;
            tmp.pop();
        }


        public:
        void addedge(mirror::Object* u,mirror::Object* v){
            // if(_set.size()==0)return;
            father[u].push_back(v);
            ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
            int t = clock();
            std::string s = u->PrettyTypeOf();
            if(_set2.count(s)){
                // ALOGD("ZHANG,need to check class: %s ",s.c_str());
                // for(auto c:_set2) ALOGD("ZHANG, string in set: %s ",c.c_str());
                int code = u->IdentityHashCode();
                t = clock() - t;
                code_time += t; 
                if(work(code,u)){
                    // _set2.erase(_set2.find(s));
                }
                
            }
        
            // ALOGD("ZHANG,addedge %s to %s",u->PrettyTypeOf().c_str(),v->PrettyTypeOf().c_str());
        }

        void clear(){
            _set2.clear();
            _set.clear();
            father.clear();
        }
        
        

        void dump_stk(){
            std::string ret="";
            ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
            while (!tmp.empty()){
                auto it = tmp.top();
                tmp.pop();
                int t = clock();
                
                std::stringstream st;
                st<<it->IdentityHashCode();
                std::string num="";
                st>>num;
                ret+=it->PrettyTypeOf()+"("+num+") <--- ";
                t = clock() - t;
                name_time += t; 

            }
            ALOGD("ZHANG, chian of ref is :  %s",ret.c_str());
            return;
        }

        int work(int hashcode,mirror::Object* start){
            if(_set.count(hashcode)==0)return 0;
            // _set.erase(hashcode);
            int deep=1;
            flag=0;
            while (1){
                while (!tmp.empty()) tmp.pop();
                vis.clear();
                dfs(start,0,deep);
                if(flag==1)break;
                deep+=1;
            }
            dump_stk();
            return 1;
        }

        void addcode(int x){
            ALOGD("ZHANG,ADD code: %d",x);
            _set.insert(x);
        }

        void addname(std::string x){
            ALOGD("ZHANG,ADD name: %s",x.c_str());
            _set2.insert(x);
        }

        int getsize(){
            return father.size();
        }

    }finder;

        void dump_info(){
            // std::fstream F;
            // F.open("/data/local/tmp/track");
            // F<<cnt++<<" hello_world\n";
            // F.close();
            ALOGD("ZHANG,THIS IS LOG");
        }

        
        static bool GetProcNameByPid(char *buf, size_t size, int pid) {
            int t = clock();
            snprintf(buf, size, "/proc/%d/cmdline", pid);
            FILE *fp = fopen(buf, "r");
            if (fp) {
                char tmp[LEN];
                if (fread(tmp, sizeof(char), size, fp))
                strcpy(buf, tmp);
                fclose(fp);
                t = clock() - t;
                read_time += t; 
                return true;
            }
            return false;
        }


        void check_file(){
            if(!istrace) return;
            // char name[LEN] = "noname";
            // GetProcNameByPid(name, LEN, getpid());
            // if(strstr(name,"myapp")==NULL) return;
            
            int t = clock();

            auto s = io.get_int();
            for(int start:s)
                finder.addcode(start);
            auto n = io.get_str();
            for(auto start:n)
                finder.addname(start);
            t = clock() - t;
            //map_time += t;
        }

        //do nothing 
        void dump_vct(){
            // char name[LEN] = "noname";
            // GetProcNameByPid(name, LEN, getpid());
            // if(strstr(name,"myapp")==NULL)
            //     return;
        }

        void addedge(mirror::Object *obj,mirror::Object *ref){
            if(!istrace) return;
            // char name[LEN] = "noname";
            // GetProcNameByPid(name, LEN, getpid());
            int t = clock();
            // if(strstr(name,"myapp")!=NULL)
            objcnt++;
            finder.addedge(ref,obj);
            t = clock() - t;
            map_time += t;
        }

        void dump_str(std::string S){
            if(!istrace) return;
            // char name[LEN] = "noname";
            // GetProcNameByPid(name, LEN, getpid());
            // if(strstr(name,"myapp")!=NULL)
            ALOGD("ZHANG, %s ",S.c_str());

        }

        void dump_init(){
            if(!istrace) return;
            char name[LEN] = "noname";
            GetProcNameByPid(name, LEN, getpid());
            ALOGD("ZHANG, %s is init! init !~~~!~~~!",name);
        }
         
        // void dump_obj(mirror::Object *obj, std::string s){
        //     //  char name[LEN] = "noname";
            
        //     // GetProcNameByPid(name, LEN, getpid());
        //     // if(strstr(name,"myapp")==NULL) return;
        //     // ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
        //     // std::string sname=obj->PrettyTypeOf();
            
        //     // ALOGD(" ZHANG,GC-OBJ %s,HASHCODE: %d ,INFO: %s!",sname.c_str(),obj->IdentityHashCode(),s.c_str());
        // }

        void gc_begin(){
            char name[LEN] = "noname";
            GetProcNameByPid(name, LEN, getpid());
            
            
            std::string str = io.get_app();
            if(strstr(name,str.c_str())!=NULL){
                ALOGD("ZHANG, GC_BEGIN ,and app name is trace:%s",name);
                istrace = true; 
            }
            else
            {
                ALOGD("ZHANG, GC_BEGIN ,but this app is not to trace:%s",name);
                /* code */
            }
            
                
            if(!istrace) return;

            read_time = 0;
            gc_time = clock();
            name_time = code_time = map_time = 0;
            finder.clear();
            objcnt=0;
            check_file();
            // ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
            // Thread* self = Thread::Current();
            // auto at = Runtime::Current()->GetClassLinker()->FindClass(self, "android/app/activity",
            //                                           ScopedNullHandle<mirror::ClassLoader>());
            // at = nullptr;
            // ALOGD("ZHANG, GC_BEGIN ");
        }

        void gc_end(){
            // char name[LEN] = "noname";
            // GetProcNameByPid(name, LEN, getpid());
            if(!istrace) return;
            // gc_time = clock() - gc_time;
            istrace = false; 
            ALOGD("ZHANG, GC_END,read_time: %d, GC_time: %d, map_time: %d ,name: %d ,code: %d!,objcnt: %d",read_time,gc_time,map_time,name_time,code_time,finder.getsize());
            finder.clear();
        }
    }
}
#undef LOG_TAG