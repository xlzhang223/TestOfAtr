
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


            std::vector<int> myio::get_int(){
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
                        //fp_int++;
                    };
                    fclose(fp);
                    // fp = fopen("/storage/self/primary/Download/hash.txt","w");
                    // if(fp) fclose(fp);
                    
                }
                return v;
            }
            std::vector<std::string> myio::get_str(){
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
                        //fp_str++;
                    };
                    fclose(fp);
                    // fp = fopen("/storage/self/primary/Download/name.txt","w");
                    // if(fp) fclose(fp);
                }
                return v;

            }
            std::string myio::get_app(){
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
            void myio::put_ans(std::string ans){
                ans+="\n";
                FILE *fp = fopen("/storage/self/primary/Download/result.txt","a");
                if(fp){
                    fwrite(ans.c_str(),sizeof(char),ans.size(),fp);
                    fclose(fp);
                }
                return;
            }

            void Leaktrace::set_main_begin(uint64_t _begin){
                main_begin=_begin;
            };
            void Leaktrace::set_main_end(uint64_t _end){
                main_end=_end;
            };
            void Leaktrace::set_heap_end(uint64_t _end){
                heap_end=_end;
            };
            uint64_t Leaktrace::get_main_begin(){
                return main_begin;
            };
            uint64_t Leaktrace::get_main_end(){
                return main_end;
            };
            uint64_t Leaktrace::get_heap_end(){
                return heap_end;
            };
            //std::unordered_map<uint64_t,uint64_t> addr_pc,addr_pc_t;

            void Leaktrace::dump_info(){
                // std::fstream F;
                // F.open("/data/local/tmp/track");
                // F<<cnt++<<" hello_world\n";
                // F.close();
                // ALOGD("ZHANG,THIS IS LOG");
            }

            
   

            void Leaktrace::check_file(){
                if(!istrace) return;
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                // if(strstr(name,"myapp")==NULL) return;
                
                // int t = clock();

                // auto s = io.get_int();
                // for(int start:s)
                //     finder.addcode(start);
                // auto n = io.get_str();
                // for(auto start:n)
                //     finder.addname(start);
                // t = clock() - t;
                //map_time += t;
            }

            //do nothing 
            void Leaktrace::dump_vct(){
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                // if(strstr(name,"myapp")==NULL)
                //     return;
            }

            void Leaktrace::addedge(mirror::Object *obj){
                if(!istrace || !((GC_K >> 2) & 1)) return;
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                // int t = clock();
                // // if(strstr(name,"myapp")!=NULL)
                // objcnt++;
                // finder.addedge(ref,obj);
                // t = clock() - t;
                // map_time += t;
                 if(Runtime::Current()-> GetHeap()==nullptr) return;
                uint64_t o = reinterpret_cast<uint64_t>((void*)obj);
                if(o<main_begin||o>=main_end) return;
                //ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                int p=0;
                if(addr_pc.count(o))
                    p=addr_pc[o];
                
                auto ptr = Runtime::Current()-> GetHeap()-> main_access_bitmap_ .get();
                if(p > 0 && ptr != nullptr){
                        if(ptr ->Test(obj)){
                            // addr_pc_t[o]=p;
                            ptr ->Clear(obj);
                        //this obj is not touch during k gc
                        //LOG(WARNING)<<"ZHANG,this "<<reinterpret_cast<uint64_t>((void*)obj)<<" NOTLEAK "<<
                        //     "and alloc at"<<p;
                        }
                        else{
                        /* code */
                            if(s_obj.count(o)){
                                s_ans.insert(o);
                            }
                            else{
                                s_obj.insert(o);
                            }
                           // LOG(WARNING)<<"ZHANG,this "<<o<<" LEAK and alloc at"<<p;
                        }
                    
                }
            }

            void Leaktrace::dump_str(std::string S){
                return;
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                // if(strstr(name,"myapp")!=NULL)
                ALOGD("ZHANG, %s ",S.c_str());

            }

            void Leaktrace::dump_init(){
                if(!istrace) return;
                char name[LEN] = "noname";
                GetProcNameByPid(name, LEN, getpid());
                // ALOGD("ZHANG, %s is init! init !~~~!~~~!",name);
            }
            
            // void dump_obj(mirror::Object *obj, std::string s){
            //     //  char name[LEN] = "noname";
                
            //     // GetProcNameByPid(name, LEN, getpid());
            //     // if(strstr(name,"myapp")==NULL) return;
            //     // ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
            //     // std::string sname=obj->PrettyTypeOf();
                
            //     // ALOGD(" ZHANG,GC-OBJ %s,HASHCODE: %d ,INFO: %s!",sname.c_str(),obj->IdentityHashCode(),s.c_str());
            // }
            void Leaktrace::heap_init(){
                char name[LEN] = "noname";
                GetProcNameByPid(name, LEN, getpid());
                
                std::string str = io.get_app();
                if(strstr(name,str.c_str())!=NULL){
                    ALOGD("ZHANG, heap_BEGIN ,and app name is trace:%s   ",name);
                    LOG(WARNING)<<"zhang i want X:"<<sizeof(uintptr_t)*8;
                    istrace = true; 
                    addr_pc.clear();
                }
                else
                {
                    ALOGD("ZHANG, heap_BEGIN ,but this app is not to trace:%s",name);
                    /* code */
                }

            }

            bool Leaktrace::get_istrace(){
                return istrace;
            }

            void Leaktrace::log_app_name(){
                char name[LEN] = "noname";
                GetProcNameByPid(name, LEN, getpid());
                
                std::string str = io.get_app();
                if(strstr(name,str.c_str())!=NULL){
                    ALOGD("ZHANG, LOG ,and app name is trace:%s",name);
                    istrace = true; 
                    // addr_pc.clear();
                }
                else{
                    //ALOGD("ZHANG, LOG ,but this app is not to trace:%s",name);
                    /* code */
                
                }

            }

            void Leaktrace::gc_begin(){
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                // std::string str = io.get_app();
                // if(strstr(name,str.c_str())!=NULL){
                //     ALOGD("ZHANG, GC_BEGIN ,and app name is trace:%s",name);
                //     istrace = true; 
                // }
                // else
                // {
                //     ALOGD("ZHANG, GC_BEGIN ,but this app is not to trace:%s",name);
                //     /* code */
                // }
                if(!istrace) return;
                ALOGD("ZHANG, GC_BEGIN and this is %d",GC_K+1);
                GC_K++;
                // finder.setpack(str);
                // read_time = 0;
                // gc_time = clock();
                // name_time = code_time = map_time = 0;
                // finder.clear();
                // objcnt=0;
                // check_file();
                // std::string sname(name);
                // sname+=" : ";
                // io.put_ans(sname);
                // ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                // Thread* self = Thread::Current();
                // auto at = Runtime::Current()->GetClassLinker()->FindClass(self, "android/app/activity",
                //                                      f(work(code,u)){
                    //     // _set2.erase(_set2.find(s));
                    // }     ScopedNullHandle<mirror::ClassLoader>());
                // at = nullptr;
            }

            void Leaktrace::gc_end(){
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                if(!istrace) return;

                if((GC_K>>2)&1){
                    //TODO:
                    /*dump something */
                    ALOGD("ZHANG, GC_END and this is %d",GC_K);
                    // swap(addr_pc_t,addr_pc);
                    // addr_pc_t.clear();
                    //ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                    for(auto it:s_ans){
                        if(addr_pc.count(it))
                            LOG(WARNING)<<"ZHANG,this "<<it<<" LEAK and alloc at"<<addr_pc[it];
                    }
                    s_ans.clear();
                    s_obj.clear();
                }
                

                // gc_time = clock() - gc_time;
                // istrace = false; 
                // // ALOGD("ZHANG, GC_END,read_time: %d, GC_time: %d, map_time: %d ,name: %d ,code: %d!,objcnt: %d",read_time,gc_time,map_time,name_time,code_time,finder.getsize());
                // finder.work();
                // finder.clear();
            }

            void Leaktrace::dump_str_ui32(mirror::Object *I){
                if(!istrace) return;
                if( I == nullptr) return;
                if(Runtime::Current()-> GetHeap()==nullptr) return;
                if((uint64_t)I<main_begin||(uint64_t)I>=main_end) return;
                //uint64_t i=reinterpret_cast<uint64_t>((void*)I);
                if(Runtime::Current()-> GetHeap()-> main_access_bitmap_.get()!=nullptr){
                    Runtime::Current()-> GetHeap()-> main_access_bitmap_ -> Set(I);
                    //ALOGD("ZHANG, %s touch %d",S.c_str(),(int)i);
                }
            
            }

            void Leaktrace::dump_str_i_i(mirror::Object *I,mirror::Object *J){
                if(!istrace) return;
                if(I==nullptr||J==nullptr) return;
                if((uint64_t)J<main_begin||(uint64_t)J>=main_end) return;
                if((uint64_t)I<main_begin||(uint64_t)I>=main_end) return;
                if(Runtime::Current()-> GetHeap()==nullptr) return;
                //ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                auto ptr = Runtime::Current()-> GetHeap()-> main_access_bitmap_.get();
                if(ptr!=nullptr){
                        if(ptr->Test(I)){
                            ptr -> Set(J);
                            ptr -> Clear(I);
                            // uint64_t i=reinterpret_cast<uint64_t>((void*)I);//from
                            // uint64_t j=reinterpret_cast<uint64_t>((void*)J);//to
                            int _to=0;
                            if(addr_pc.count(reinterpret_cast<uint64_t>((void*)I)))
                                _to=addr_pc[reinterpret_cast<uint64_t>((void*)I)];
                            if(_to!=0){
                                addr_pc[reinterpret_cast<uint64_t>((void*)J)]=_to;
                                addr_pc.erase(reinterpret_cast<uint64_t>((void*)I));
                                //addr_pc[reinterpret_cast<uint64_t>((void*)I)]=0;
                            }
                        }
                }
            }

            void Leaktrace::new_obj(mirror::Object *obj){
                if(!istrace) return;
                //obj->
                if(obj == nullptr ) return;
                uint64_t o = reinterpret_cast<uint64_t>((void*)obj);
                if(o<main_begin||o>=main_end) return;
                //ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                uint64_t pc = Thread::Current()-> GetAllocSite();
                if(pc>0)
                    addr_pc[o]=pc;
                //LOG(WARNING)<<"ZHANG,new obj "<<reinterpret_cast<uint64_t>((void*)obj)<<" pc is "<<pc;
            }
        }
        
        
        
        
    
        // class Finder{
        //     private:
        //     const int max_deep=16;
        //     std::unordered_map<mirror::Object*,std::vector<mirror::Object*>> father;
        //     // std::unordered_map<int,mirror::Object*> map_;
        //     std::unordered_set<int> _set;
        //     std::unordered_set<std::string> _set2;

        //     std::stack<mirror::Object*> tmp;
        //     std::unordered_set<mirror::Object*> vis;
        //     std::string pack;
        //     int flag=0;
        //     std::unordered_map<int,mirror::Object*> to_do;

        //     void dfs(mirror::Object* now,int dp,int deep,int p_num){
        //         if(dp>deep||flag==1) return;
        //         tmp.push(now);
        //         vis.insert(now);
        //         ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
        //         std::string s = now->PrettyTypeOf();
        //         int add=0;
        //         if(s.find(pack)!= std::string::npos)add++;
        //         if(father[now].size()==0&&dp == deep){
        //             vis.erase(now);
        //             if(p_num<2) return;
        //             flag=1;
        //             return;
        //         }
        //         int nxt = 0;
        //         for(auto next:father[now]){
        //             if(vis.count(next)==0)
        //                 dfs(next,dp+1,deep,p_num+add),++nxt;
        //         }
        //         if(nxt==0&&dp == deep&&p_num>1){
        //             flag=1;
        //         }
        //         vis.erase(now);
        //         if(flag==1) return;
        //         tmp.pop();
        //     }


        //     public:
        //     void addedge(mirror::Object* u,mirror::Object* v){
        //         // if(_set.size()==0)return;
        //         father[u].push_back(v);

        //         ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
        //         int t = clock();
        //         std::string s = u->PrettyTypeOf();
        //         if(_set2.count(s)){
        //             // ALOGD("ZHANG,need to check class: %s ",s.c_str());
        //             // for(auto c:_set2) ALOGD("ZHANG, string in set: %s ",c.c_str());
        //             int code = u->IdentityHashCode();
        //             t = clock() - t;
        //             code_time += t; 

        //             to_do[code]=u;
        //             // if(work(code,u)){
        //             //     // _set2.erase(_set2.find(s));
        //             // }
        //         }
            
        //         // ALOGD("ZHANG,addedge %s to %s",u->PrettyTypeOf().c_str(),v->PrettyTypeOf().c_str());
        //     }

        //     void clear(){
        //         _set2.clear();
        //         _set.clear();
        //         to_do.clear();
        //         father.clear();
        //     }
            
            

        //     void dump_stk(){
        //         std::string ret=pack+":\n";
        //         ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
        //         while (!tmp.empty()){
        //             auto it = tmp.top();
        //             tmp.pop();
        //             int t = clock();
                    
        //             std::stringstream st;
        //             st<<it->IdentityHashCode();
        //             std::string num="";
        //             st>>num;
        //             ret+=it->PrettyTypeOf()+"("+num+") <---\n";
        //             t = clock() - t;
        //             name_time += t; 

        //         }
        //         io.put_ans(ret);
        //         // ALOGD("ZHANG, chian of ref is :  %s",ret.c_str());
        //         return;
        //     }

        //     int work(int hashcode,mirror::Object* start){
        //         if(_set.count(hashcode)==0)return 0;
        //         int deep=1;
        //         flag=0;
        //         while (deep<max_deep){
        //             while (!tmp.empty()) tmp.pop();
        //             vis.clear();
        //             dfs(start,0,deep,0);
        //             // dfs(start);
        //             if(flag==1){
        //                 flag=0;
        //                 dump_stk();
        //                 break;
        //             }
        //             deep+=1;
        //         }
                
        //         return 1;
        //     }

        //     void work(){
        //         for(auto it:to_do){
        //             work(it.first,it.second);
        //         }
        //     }

        //     void addcode(int x){
        //         // ALOGD("ZHANG,ADD code: %d",x);
                
        //         _set.insert(x);
        //     }

        //     void addname(std::string x){
        //         // ALOGD("ZHANG,ADD name: %s",x.c_str());
        //         _set2.insert(x);
        //     }

        //     int getsize(){
        //         return father.size();
        //     }

        //     void setpack(std::string str){
        //         pack=str;
        //     }

        // }finder;


        
        
    
}
#undef LOG_TAG