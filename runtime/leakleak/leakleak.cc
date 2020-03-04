
#define LOG_TAG "leakleak"
#include "leakleak.h"
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
#include <fstream>
#include <sys/msg.h>

#include "gc/heap.h"
#include "runtime.h"
#include "gc/space/large_object_space.h"
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
    namespace gc {
        class Heap;
        namespace accounting {
            template <size_t kAlignment> class SpaceBitmap;
            typedef SpaceBitmap<kObjectAlignment> ContinuousSpaceBitmap;
            class HeapBitmap;
        } 
    }
    namespace leakleak{
            
            static constexpr size_t kRegionSize = 256 * KB;

            int mod_k = 16;
            int need_log =0 ;
            
            std::mutex my_lock2;
            std::map<uintptr_t,int>  S_pc;
            std::map<mirror::Object *,int>  pc_cnt;
            std::map<uintptr_t,std::string> pc_method;
            std::map<uintptr_t,std::string> pc_class;
            int count_obj = 0;
            int tol_size =0;
            int find_new=0;
            int scan_cnt = 0;
            int try_cnt = 0;
            int move_cnt = 0;
            int move_cnt2 = 0;
            int is_full = 0;
            static Leaktrace* instance_ = new Leaktrace();
            Leaktrace* getInstance(){
                return instance_;
            }
            // const int MB = 1024 *1024;
            std::vector<int> myio::get_int(std::string path){
                std::vector<int> v;
                int ret = -1;
                char tmp[len];
                FILE *fp = fopen(path.c_str(),"r");
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
            std::vector<std::string> myio::get_app(){
                std::vector<std::string> v;
                // v.push_back("zygote");
                // v.push_back("launcher");
                // v.push_back("_server");
                std::string ret = "-1";
                char tmp[len];
                FILE *fp = fopen("/data/local/tmp/app.txt","r");
                if(fp){
                    memset(tmp,0,sizeof(tmp));
                    while(fread(tmp,sizeof(char),len,fp)){
                        std::stringstream ss(tmp);
                        while(ss>>ret)v.push_back(ret);
                        //ss>>ret;
                    };
                    fclose(fp);
                    // fp = fopen("/storage/self/primary/Download/name.txt","w");
                    // if(fp) fclose(fp);
                }
                // return ret;
                return v;
            }
            void myio::put_ans(std::string ans){
                ans+="\n";
                FILE *fp = fopen("/data/local/tmp/result.txt","a");
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
            void Leaktrace::new_map(){
                if(!try_thread_istrace()) return;
                map_cnt = (main_end - main_begin) / kRegionSize;
                my_lock = new std::mutex[map_cnt+1];
                // my_lock.lock();
                addr_pc = new std::unordered_map<mirror::Object *,std::pair<uintptr_t,uint16_t>>[map_cnt];
                // my_lock.unlock();
            };
            void Leaktrace::clear_map(size_t idx){
                if(!try_thread_istrace()) return;
                //map_cnt = (main_end - main_begin) / kRegionSize;
                my_lock[idx].lock();
                addr_pc[idx].clear(); //= new std::unordered_map<uint32_t,std::pair<uintptr_t,uint16_t>>[map_cnt];
                my_lock[idx].unlock();
            };
            size_t Leaktrace::get_obj_idx(mirror::Object *obj){
                uintptr_t offset = reinterpret_cast<uintptr_t>(obj) - reinterpret_cast<uintptr_t>(main_begin);
                size_t reg_idx = offset / kRegionSize;
                return reg_idx;
            }
            void Leaktrace::set_bitmap_ptr(uint64_t _end){
                // LOG(WARNING)<<"Leakleak,set_bitmap_ptr: "<<_end ;
                bitmap_ptr=_end;
            };
            uint64_t Leaktrace::get_main_begin(){
                return main_begin;
            };
            uint64_t Leaktrace::get_main_end(){
                return main_end;
            };
            uint64_t Leaktrace::get_bitmap_ptr(){
                // LOG(WARNING)<<"Leakleak,get_bitmap_ptr: "<<bitmap_ptr ;
                return bitmap_ptr;
            };
            //std::unordered_map<uint64_t,uint64_t> addr_pc,addr_pc_t;

            void Leaktrace::dump_info(uint64_t tim){
                if(!try_thread_istrace()) return;
                if(need_log)LOG(WARNING)<<"Leakleak,info about gc time: "<<tim ;
                // std::fstream F;
                // F.open("/data/local/tmp/track");
                // F<<cnt++<<" hello_world\n";
                // F.close();
                // ALOGD("Leakleak,THIS IS LOG");
            }

            
   

            void Leaktrace::check_file(){
                if(!try_thread_istrace()) return;
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



            void Leaktrace::addedge(mirror::Object *obj,gc::collector::GcType ty,gc::Heap* heap_){
                
                // if(!try_thread_istrace()){
                //     // my_lock.unlock();
                //     return;
                // }
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                // int t = clock();
                // // if(strstr(name,"myapp")!=NULL)
                // objcnt++;
                // finder.addedge(ref,obj);
                // t = clock() - t;
                // map_time += t;
                    // my_lock.unlock();
                   // LOG(WARNING)<< "Leakleak,addegde but something wrong with heap";
                //int gc = Thread::Current()-> GetGCnum();
                if(GC_K==0 || GC_K%mod_k!=0) return;
                if (ty != gc::collector::kGcTypePartial)
                {
                //     ALOGD("Leakleak, addedge ,kGcTypePartial ");
                // }
                // else if (ty == gc::collector::kGcTypeSticky)
                // {
                //     ALOGD("Leakleak, addedge ,kGcTypeSticky  ");
                    return;
                }
                scan_cnt ++;
                if(heap_ == nullptr){
                    return;
                }
                auto ptr = heap_-> main_access_bitmap_.get();
                if(ptr==nullptr||!ptr->HasAddress(obj)){
                    return;
                }//ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                std::pair<uint64_t,uint16_t> p={0,0};
                try_cnt ++;
                size_t idx = get_obj_idx(obj);
                my_lock[idx].lock();
                if(addr_pc[idx].count(obj)){
                    p=addr_pc[idx][obj];
                    addr_pc[idx][obj].second=GC_K;
                    my_lock[idx].unlock();
                    {
                        ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                        std::string class_name = obj->GetClass() -> PrettyDescriptor();
                        LOG(WARNING)<< "Leakleak,addegde class in hashtable: " << class_name <<" class pc:"<<(void*)p.first << " gc:" << GC_K <<" last gc:"<<p.second<<" and this is pc-cnt: "<<pc_cnt[obj];
                    }
                }
                else{
                    {
                        ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                        std::string class_name = obj->GetClass() -> PrettyDescriptor();
                        LOG(WARNING)<< "Leakleak,addegde class not in hashtable: " << class_name ;//<<" class pc:"<<(void*)p.first << " gc:" << gc <<" last gc:"<<p.second;
                    }
                    my_lock[idx].unlock();
                    return;
                }
                //auto ptr = Runtime::Current()-> GetHeap()-> main_access_bitmap_ .get();
                if(p.first != 0 && ptr != nullptr && GC_K - p.second >= mod_k){
                        my_lock[idx].lock();
                        if(ptr ->Test(obj)){
                            // addr_pc_t[o]=p;
                            // vis[low(obj)]=p.second;
                            ptr ->Clear(obj);
                            //this obj is not touch during k gc
                            LOG(WARNING)<<"Leakleak,this "<<reinterpret_cast<uint64_t>((void*)obj)<<" NOTLEAK "<<
                            "and alloc at"<<(void*)p.first<<" and this is pc-cnt: "<<pc_cnt[obj];
                        }
                        else{// && vis.count(low(obj)) == 0){
                        /* code */
                            //s_ans.insert(p);
                            // vis[low(obj)]=p.second;
                            my_lock2.lock();
                            ++m_ans[p.first];
                            my_lock2.unlock();
                            // LOG(WARNING)<<"Leakleak,this "<<reinterpret_cast<uint64_t>((void*)obj)<<" LEAK "<<
                            // "and alloc at"<<(void*)p.first;
                            // {
                            //     ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                            //     std::string class_name = obj->GetClass() -> PrettyDescriptor();
                            //     LOG(WARNING)<< "Leakleak,find leak class: " << class_name <<" class pc:"<<(void*)p.first << " gc:" << gc <<" last gc:"<<p.second;;
                            // }
                           LOG(WARNING)<<"Leakleak,this "<<reinterpret_cast<uint64_t>((void*)obj)<<" LEAK and alloc at"<<(void*)p.first<<" and this is pc-cnt: "<<pc_cnt[(obj)];;
                        }
                        my_lock[idx].unlock();
                }
                // else{
                //     {
                //         ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                //         std::string class_name = obj->GetClass() -> PrettyDescriptor();
                //         LOG(WARNING)<< "Leakleak,addegde class not try : " << class_name <<" class pc:"<<(void*)p.first << " gc:" << gc <<" last gc:"<<p.second;
                //     }
                // }
                {
                    return;
                }
            }

            void Leaktrace::dump_str(std::string S){
                // return;
                if(!try_thread_istrace()) return;
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                // if(strstr(name,"myapp")!=NULL)
                ALOGD("Leakleak,info: %s ",S.c_str());

            }

            void Leaktrace::dump_init(){
                if(!try_thread_istrace()) return;
                char name[LEN] = "noname";
                GetProcNameByPid(name, LEN, getpid());
                // ALOGD("Leakleak, %s is init! init !~~~!~~~!",name);
            }
            
            // void dump_obj(mirror::Object *obj, std::string s){
            //     //  char name[LEN] = "noname";
                
            //     // GetProcNameByPid(name, LEN, getpid());
            //     // if(strstr(name,"myapp")==NULL) return;
            //     // ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
            //     // std::string sname=obj->PrettyTypeOf();
                
            //     // ALOGD(" Leakleak,GC-OBJ %s,HASHCODE: %d ,INFO: %s!",sname.c_str(),obj->IdentityHashCode(),s.c_str());
            // }
            int Leaktrace::try_thread_istrace(){
                return istrace;
                // if(Thread::Current() == nullptr){
                //     // return 0;
                //     char name[LEN] = "noname";
                //     GetProcNameByPid(name, LEN, getpid());
                //      //LOG(WARNING)<<"Leakleak,pname: "<<name<<" pid"<<getpid();
                //     std::vector<std::string> v = io.get_app();
                //     int f = 0;
                //     for(auto str: v){
                //         if(strstr(name,str.c_str())!=NULL){
                //             f=1;
                //         }
                //     }
                //         LOG(WARNING)<<"Leakleak,info about offset: "<< Thread::AllocSiteOffset<PointerSize::k64>().Int32Value();
                //     if(f){
                //         // is = 1;
                //         ALOGD("Leakleak, heap_BEGIN no thread,and app name is trace:%s   ",name);
                //         // LOG(WARNING)<<"zhang i want X:"<<sizeof(uintptr_t)*8;
                //         istrace = true; 
                //         // addr_pc.clear();
                //         auto vv = io.get_int("/data/local/tmp/GC_K.txt");
                //         if(vv.size()>0){
                //             mod_k = vv[0];
                //         }
                //         vv = io.get_int("/data/local/tmp/Tsize.txt");
                //         if(vv.size()>0){
                //             t_size = vv[0];
                //         }
                //         // Thread::Current()-> Setistrace(1);
                //         return 1;
                //     }
                //     else
                //     {
                //         // is = 0;
                //         // Thread::Current()-> Setistrace(0);
                //         ALOGD("Leakleak, heap_BEGIN no thread,but this app is not to trace:%s",name);
                //         /* code */
                //         return 0;
                //     }
                // }
                // int is = Thread::Current()-> Getistrace();
                // if(is == -1){
                //     // return 0;
                //     // std::string t<"Leakleak,hhread_name;
                //     // Thread::Current() ->GetThreadName(thread_name);
                //     // LOG(WARNING)<ave a try of ThreadName:"<<thread_name;

                //     char name[LEN] = "noname";
                //     GetProcNameByPid(name, LEN, getpid());
                //     // std::string s(name);
                //    // LOG(WARNING)<<"Leakleak,pname: "<<name<<" pid"<<getpid();
                //     std::vector<std::string> v = io.get_app();
                //     int f = 0;
                //     for(auto str: v){
                //         if(strstr(name,str.c_str())!=NULL){
                //             f=1;
                //         }
                //     }
                //     if(f){
                //         is = 1;
                //         ALOGD("Leakleak, Thread_BEGIN ,and app name is trace:%s   ",name);
                //         // LOG(WARNING)<<"zhang i want X:"<<sizeof(uintptr_t)*8;
                //         istrace = true; 
                //         // addr_pc.clear();
                //         auto vv = io.get_int("/data/local/tmp/GC_K.txt");
                //         if(vv.size()>0){
                //             mod_k = vv[0];
                //         }
                //         vv = io.get_int("/data/local/tmp/Tsize.txt");
                //         if(vv.size()>0){
                //             t_size = vv[0];
                //         }
                //         Thread::Current()-> Setistrace(1);
                //     }
                //     else
                //     {
                //         is = 0;
                //         Thread::Current()-> Setistrace(0);
                //         ALOGD("Leakleak, Thread_BEGIN ,but this app is not to trace:%s",name);
                //         /* code */
                //     }
                // }
                // return is;
            }
            
            void Leaktrace::heap_init(){
                char name[LEN] = "noname";
                GetProcNameByPid(name, LEN, getpid());
                    //LOG(WARNING)<<"Leakleak,pname: "<<name<<" pid"<<getpid();
                std::vector<std::string> v = io.get_app();
                int f = 0;
                // v.push_back("zygote64");
                // v.push_back("system_server");
                for(auto str: v){
                    if(strstr(name,str.c_str())!=NULL){
                        f=1;
                    }
                }
                 LOG(WARNING)<<"Leakleak,info about offset: "<< Thread::AllocSiteOffset<PointerSize::k64>().Int32Value();
                if(f){
                    // is = 1;
                    ALOGD("Leakleak, heap_BEGIN ,and app name is trace:%s   ",name);
                    // LOG(WARNING)<<"zhang i want X:"<<sizeof(uintptr_t)*8;
                    istrace = true; 
                    // addr_pc.clear();
                    auto vv = io.get_int("/data/local/tmp/GC_K.txt");
                    if(vv.size()>0){
                        mod_k = vv[0];
                    }
                    vv = io.get_int("/data/local/tmp/Tsize.txt");
                    if(vv.size()>0){
                        t_size = vv[0];
                    }
                    // Thread::Current()-> Setistrace(1);
                    // return 1;
                }
                else{
                    ALOGD("Leakleak, heap_BEGIN no ,and app name is trace:%s   ",name);
                }
                if(!try_thread_istrace())return;
                
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                
                // std::string str = io.get_app();
                // if(strstr(name,str.c_str())!=NULL){
                //     ALOGD("Leakleak, heap_BEGIN ,and app name is trace:%s   ",name);
                //     LOG(WARNING)<<"zhang i want X:"<<sizeof(uintptr_t)*8;
                //     istrace = true; 
                //     addr_pc.clear();
                //     auto v = io.get_int("/storage/self/primary/Download/GC_K.txt");
                //     if(v.size()>0){
                //         mod_k = v[0];
                //     }
                //     v = io.get_int("/storage/self/primary/Download/Tsize.txt");
                //     if(v.size()>0){
                //         t_size = v[0];
                //     }
                    
                // }
                // else
                // {
                //     ALOGD("Leakleak, heap_BEGIN ,but this app is not to trace:%s",name);
                //     /* code */
                // }

            }



            bool Leaktrace::get_istrace(){
                return try_thread_istrace();
            }

            void Leaktrace::log_app_name(){
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                
                // std::string str = io.get_app();
                // if(strstr(name,str.c_str())!=NULL){
                //     ALOGD("Leakleak, LOG ,and app name is trace:%s",name);
                //     istrace = true; 
                //     // addr_pc.clear();
                // }
                // else{
                //     //ALOGD("Leakleak, LOG ,but this app is not to trace:%s",name);
                //     /* code */
                
                // }

            }

            void Leaktrace::gc_begin(gc::collector::GcType ty){
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                // std::string str = io.get_app();
                // if(strstr(name,str.c_str())!=NULL){
                //     ALOGD("Leakleak, GC_BEGIN ,and app name is trace:%s",name);
                //     istrace = true; 
                // }
                // else
                // {
                //     ALOGD("Leakleak, GC_BEGIN ,but this app is not to trace:%s",name);
                //     /* code */
                // }
                if(!try_thread_istrace()) return;
                if (ty == gc::collector::kGcTypePartial)
                {
                    // ALOGD("Leakleak, GC_BEGIN and this is %d",GC_K+1);
                    GC_K++;
                    Thread::Current()-> SetGCnum(GC_K);
                    //isFull = true;
                    ALOGD("Leakleak, GC_BEGIN ,kGcTypePartial and this is:%d",GC_K);
                }
                // else if (ty == gc::collector::kGcTypeSticky)
                // {
                //     Thread::Current()-> SetGCnum(GC_K);
                //     ALOGD("Leakleak, GC_BEGIN ,kGcTypeSticky  ");
                // }
                else{
                    Thread::Current()-> SetGCnum(GC_K);
                }
                
                // if(GC_K % mod_k == 0){
           
                //     if(ty== gc::collector::kGcTypeSticky){
                //             ALOGD("Leakleak, GC_BEGIN ,kGcTypeSticky  ");
                //     }
                //     else if (ty == gc::collector::kGcTypePartial)
                //     {
                //         ALOGD("Leakleak, GC_BEGIN ,kGcTypePartial  ");
                //     }
                //     else{
                //         ALOGD("Leakleak, GC_BEGIN ,something wrong with GCtype  ");
                //     }
                // }
                //if(GC_K % mod_k == 0)vis.clear();
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

            void Leaktrace::gc_end(gc::collector::GcType ty){
                // GetProcNameByPid(name, LEN, getpid());
                if(!try_thread_istrace()) return;
                //isFull = 0;
                int gc = Thread::Current()-> GetGCnum();
                gc = GC_K;
                if(gc % mod_k == 0 && ty == gc::collector::kGcTypePartial ){
                    //begin of scan obj 
                    auto ptr = Runtime::Current()-> GetHeap()-> main_access_bitmap_.get();
                    for(size_t i=0;i<map_cnt;i++){
                        my_lock[i].lock();
                        
                        for(auto it=addr_pc[i].begin();it!=addr_pc[i].end();it++){
                            auto p = it->second;
                            auto obj = (it->first);
                            // {
                            //     ReaderMutexLock mu(Thread::Current(), *Locks::heap_bitmap_lock_);
                            //     if(!Runtime::Current()-> GetHeap()->GetLiveBitmap()->Test(obj)
                            //         &&!Runtime::Current()-> GetHeap()->GetMarkBitmap()->Test(obj)) continue;
                            // }
                            if(p.first != 0 && ptr != nullptr && GC_K - p.second >= mod_k){
                                    if(ptr ->Test(obj)){
                                        // addr_pc_t[o]=p;
                                        // vis[low(obj)]=p.second;
                                        ptr ->Clear(obj);
                                        //this obj is not touch during k gc
                                        LOG(WARNING)<<"Leakleak,this "<<reinterpret_cast<uint64_t>((void*)obj)<<" NOTLEAK "<<
                                        "and alloc at"<<(void*)p.first<<" and this is pc-cnt: "<<pc_cnt[(obj)];
                                    }
                                    else{// && vis.count(low(obj)) == 0){
                                    /* code */
                                        //s_ans.insert(p);
                                        // vis[low(obj)]=p.second;
                                        // my_lock2.lock();
                                        ++m_ans[p.first];
                                        // my_lock2.unlock();
                                        // LOG(WARNING)<<"Leakleak,this "<<reinterpret_cast<uint64_t>((void*)obj)<<" LEAK "<<
                                        // "and alloc at"<<(void*)p.first;
                                        // {
                                        //     ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                                        //     std::string class_name = obj->GetClass() -> PrettyDescriptor();
                                        //     LOG(WARNING)<< "Leakleak,find leak class: " << class_name <<" class pc:"<<(void*)p.first << " gc:" << gc <<" last gc:"<<p.second;;
                                        // }
                                    LOG(WARNING)<<"Leakleak,this "<<reinterpret_cast<uint64_t>((void*)obj)<<" LEAK and alloc at"<<(void*)p.first<<" and this is pc-cnt: "<<pc_cnt[(obj)];;
                                    }
                            }

                        }
                         my_lock[i].unlock();
                    
                    }
                    // my_lock.unlock();
                    // char name[LEN] = "noname";
                    // gc_time = clock() - gc_time;
                    //TODO:
                    /*dump something */
                    // ALOGD("Leakleak, GC_END and this is %d",gc);
                    // swap(addr_pc_t,addr_pc);
                    // addr_pc_t.clear();
                    //ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                    // auto ptr = Runtime::Current()-> GetHeap()-> main_access_bitmap_ .get();
                    // for(auto it:addr_pc){
                    //     if(ptr->Test(it.first)){
                    //         ptr->Clear(it.first);
                    //     }
                    //     else{
                    //         LOG(WARNING)<<"Leakleak,this "<<it.first<<" LEAK and alloc at"<<it.second<<"and GC is "<<gc;
                    //     }
                    // }
                    char name[LEN] = "noname";
                    GetProcNameByPid(name, LEN, getpid());
                    my_lock2.lock();
                    std::stringstream stream;
                    LOG(WARNING)<<"Leakleak,info about: "<<name ;//<<" hashtable_size:"<<16* (addr_pc.size())<<" scan cnt "<<scan_cnt<<" try cnt "<<try_cnt<<" move cnt "<<move_cnt<<" success move "<<move_cnt2<<" gc: "<<gc;
                    stream <<"info about: "<<name <<":\n";
                    
                    if(ptr!=nullptr){

                        for(auto it=addr_large_pc.begin();it!=addr_large_pc.end();it++){
                            if(ptr ->Test((it->first))){
                                ptr ->Clear((it->first));
                            }
                            else{
                                ++m_ans[it->second.first];
                            }
                            it->second.second = gc;
                        }
                    }
                    
                    for(auto it:m_ans){
                            auto pc = it.first;
                            jit::Jit* jit = Runtime::Current()->GetJit();
                            if(pc > 0 && (jit!=nullptr && jit->GetCodeCache()->ContainsPc((void*)pc))){
                        
                                auto jt = pc_method.upper_bound(it.first);
                                if(jt != pc_method.begin()){
                                    jt--;
                                // if(it.second>1)
                                    LOG(WARNING)<<"Leakleak,maybe leak at "<<(void*)it.first<<" and GC is " << gc << " and from method "<< jt->second<<" and class is "<<pc_class[it.first];
                                    stream <<"maybe leak at "<<(void*)it.first<<" and GC is " << gc << " and from method "<< jt->second<<" and class is "<<pc_class[it.first] <<std::endl;

                                }
                                // my_lock2.unlock();

                            }
                            else{
                                // if(it.second>1)
                                    LOG(WARNING)<<"Leakleak,maybe leak at  "<<(void*)it.first<<" and GC is " << gc <<" and class is "<<pc_class[it.first];
                                    stream<<"maybe leak at  "<<(void*)it.first<<" and GC is " << gc <<" and class is "<<pc_class[it.first]<<std::endl;
                            }
                    }
                    m_ans.clear();
                    // for(size_t i=0;i<map_cnt;i++){
                    //     my_lock[i].lock();
                    //     std::vector<mirror::Object *> to_erase;
                    //     for(auto it=addr_pc[i].begin();it!=addr_pc[i].end();it++){
                    //         if(GC_K - it->second.second >= mod_k*2 )
                    //             to_erase.push_back(it->first);
                    //     }
                    //     for(auto it:to_erase){
                    //         addr_pc[i].erase(it);
                    //     }
                    //     my_lock[i].unlock();
                    // }
                    // for(auto it:S_pc){
                    //     auto pc = it.first;

                    //     {
                    //         ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                    //         LOG(WARNING)<< "Leakleak,pc alloc: " << pc_class[pc] <<" class pc:"<<(void*)pc << " count " << it.second ;
                    //     }
                    // }
                    LOG(WARNING)<<"Leakleak,alloc obj tol:"<<count_obj;
                    LOG(WARNING)<<"Leakleak,alloc obj byte tol:"<<tol_size;
                    LOG(WARNING)<<"Leakleak,diff pc tol:"<<S_pc.size();
                    LOG(WARNING)<<"Leakleak,find new tol:"<<find_new;

                    stream<<"alloc obj tol:"<<count_obj<<std::endl;
                    stream<<"alloc obj byte tol:"<<tol_size<<std::endl;
                    stream<<"diff pc tol:"<<S_pc.size()<<std::endl;
                    stream<<"find new tol:"<<find_new<<std::endl;
                    my_lock2.unlock();
                    io.put_ans(stream.str());
                    // LOG(WARNING)<<"Leakleak,cost mem:"<<sizeof(getInstance())+sizeof(*Runtime::Current()-> GetHeap()-> main_access_bitmap_ .get());
                    // if((gc/2) % mod_k == 0)vis.clear();
                }
                
                {
                    return;
                }
                // gc_time = clock() - gc_time;
                // istrace = false; 
                // // ALOGD("Leakleak, GC_END,read_time: %d, GC_time: %d, map_time: %d ,name: %d ,code: %d!,objcnt: %d",read_time,gc_time,map_time,name_time,code_time,finder.getsize());
                // finder.work();
                // finder.clear();
            }
            void Leaktrace::heap_end(){
                    if(!try_thread_istrace()) return;
                    LOG(WARNING)<<"Leakleak,info about: "<<" scan cnt "<<scan_cnt<<" try cnt "<<try_cnt<<" move cnt "<<move_cnt<<" success move "<<move_cnt2<<" gc_tol: "<<GC_K;
                    for(auto it:S_pc){
                        auto pc = it.first;
                        {
                            ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                            LOG(WARNING)<< "Leakleak,HEAP END pc alloc: " << pc_class[pc] <<" class pc:"<<(void*)pc << " count " << it.second ;
                        }
                    }
            }
            void Leaktrace::interpreter_touch_obj(mirror::Object *I){
                if(!try_thread_istrace()) return;
                if( I == nullptr) return;
                if(Runtime::Current()-> GetHeap()==nullptr) return;
                if((uint64_t)I<main_begin||(uint64_t)I>=main_end) return;
                //uint64_t i=reinterpret_cast<uint64_t>((void*)I);
                if(Runtime::Current()-> GetHeap()-> main_access_bitmap_.get()!=nullptr){
                    // my_lock.lock();
                    Runtime::Current()-> GetHeap()-> main_access_bitmap_ -> Set(I);
                    // my_lock.unlock();
                    //ALOGD("Leakleak, %s touch %d",S.c_str(),(int)i);
                }
            
            }
            void Leaktrace::Free_obj(mirror::Object *obj){
                if(!try_thread_istrace()) return;
                //  LOG(WARNING)<< "Leakleak,Free low: "<<low(obj);
                 if(addr_large_pc.count(obj)){
                     addr_large_pc.erase(obj);
                 }
            }
            void Leaktrace::set_bitmap(gc::accounting::HeapBitmap* _live_bitmap){
                live_bitmap = _live_bitmap;
            };
            void Leaktrace::CC_move_from_to(mirror::Object *I,mirror::Object *J,gc::Heap* heap_){
                if(!try_thread_istrace()) return;
                if(I == nullptr||J == nullptr||I == J) return;
                //ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                auto ptr = heap_-> main_access_bitmap_.get();
                size_t idx_i = get_obj_idx(I);
                size_t idx_j = get_obj_idx(J);
                if(ptr!=nullptr&&ptr->HasAddress(I)&&ptr->HasAddress(J)){
                    move_cnt++;
                    my_lock[idx_i].lock();
                    my_lock[idx_j].lock();
                        if(addr_pc[idx_i].count(I)){
                            move_cnt2++;
                            addr_pc[idx_j][J]=addr_pc[idx_i][I];
                            pc_cnt[J] = pc_cnt[I];
                            pc_cnt.erase(I);
                            addr_pc[idx_i].erase(I);
                            if(ptr->Test(I)){
                                ptr -> Set(J);
                                ptr -> Clear(I);
                            }
                        }
                    my_lock[idx_j].unlock();
                    my_lock[idx_i].unlock();
                }
                else {
                    LOG(WARNING)<< "Leakleak, move wrong!(not in ptr)";
                }
                {
                    // my_lock.unlock();
                    return;
                }
            }

            void Leaktrace::NewClass(mirror::Class *klass,void* pc) {
                if (klass && classes_.find({klass,pc}) == classes_.end()) {
                    classes_.insert({klass,pc});

                    // ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                    // std::string class_name = klass -> PrettyDescriptor();
                    //  LOG(WARNING)<< "Leakleak,update class: " << class_name <<" pc "<<pc;
                    // uint32_t obj_size = klass->GetObjectSize();
                    // // if (obj_size == 0U)
                    // //   obj_size = klass->GetClassSize();

                    // // leading character 'c' means this line is Class information
                    // // NOTE: object size may be zero if class is interface or abstract
                    // // PRINT(class_fp_, "c %8X %8u %s\n",
                    // //       reinterpret_cast<uint32_t>(klass),
                    // //       obj_size,
                    // //       class_name.c_str());

                    // class_meta_info_.push_back(ClassOrMethod(
                    //                             reinterpret_cast<uintptr_t>(klass),
                    //                             obj_size,
                    //                             class_name));

                    // // auto DumpMethods = [&](mirror::ObjectArray<ArtMethod>* array) {
                    // //   if (array) {
                    // //     int len = array->GetLength();
                    // //     for (int i = 0; i < len; ++i)
                    // //       NewMethodLinked(class_name, *(array->Get(i)));
                    // //   }
                    // // };
                    
                    
                    //     auto pointer_size = Runtime::Current()->GetClassLinker()->GetImagePointerSize();
                    //     auto direct_methods = klass->GetDirectMethods(pointer_size);
                    //     for (auto& m : direct_methods) {
                    //         NewMethodLinked(class_name, m);
                    //     }
                    //     auto virtual_methods = klass->GetVirtualMethods(pointer_size);
                    //     for (auto& m : virtual_methods) {
                    //         NewMethodLinked(class_name, m);
                    //     }
                    //     for (auto i = 0; i < klass->GetVTableLength(); i++) {
                    //         NewMethodLinked(class_name, *(klass->GetVTableEntry(i, pointer_size)));
                    //     }
                    //     // auto im_methods = klass->GetImTable();
                    //     // if (im_methods)
                    //     //   for (int i = 0; i < im_methods->GetLength(); ++i)
                    //     //     NewMethodLinked(class_name, im_methods->Get(i));
                    //     // DumpMethods(im_methods);

                    //     // interface table is an array of pair (Class*, ObjectArray<ArtMethod*>)
                    //     auto if_table = klass->GetIfTable();
                    //     auto method_array_size = klass->GetIfTableCount();
                    //     for (int i = 0; i < method_array_size; ++i) {
                    //         auto method_array = if_table->GetMethodArray(i);
                    //         for (size_t k = 0; k < if_table->GetMethodArrayCount(i); k++) {
                    //             NewMethodLinked(class_name, *(method_array->GetElementPtrSize<ArtMethod*>(k, pointer_size)));
                    //         }
                    //     }
                    
                }
            }




            void Leaktrace::NewMethodLinked(const std::string& class_name, ArtMethod &method) {
                // leading character 'm' means this line is Method information
                auto code = reinterpret_cast<uintptr_t>(method.GetEntryPointFromQuickCompiledCode());
                ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                std::string method_name(class_name + "::" + method.GetName());
                LOG(WARNING)<< "Leakleak,find class method: " << method_name << "and code: " << code;
                // PRINT(class_fp_, "m %8X %8X %s\n", code, code + method->GetCodeSize(), method_name.c_str());
                // method_meta_info_.push_back(ClassOrMethod(reinterpret_cast<unsigned int>(code),
                //                                         reinterpret_cast<unsigned int>(code) + method.GetCodeSize(),
                //                                         method_name));
            }

            void Leaktrace::new_method(ArtMethod * method){
                if(!try_thread_istrace() || method == nullptr) return;
                ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                auto code = reinterpret_cast<uintptr_t>(method->GetEntryPointFromQuickCompiledCode());
                auto klass =method->GetDeclaringClass();
                if(klass != nullptr){
                    //NewClass(klass);
                    std::string class_name = klass -> PrettyDescriptor();
                    std::string method_name(class_name + "::" + method->GetName());
                    LOG(WARNING)<< "Leakleak,update method: " << method_name << "and code: " << reinterpret_cast<uintptr_t>(code);
                }
            }

            void Leaktrace::put_jit(uint8_t* code,std::string method_name,bool osr){
                if(!try_thread_istrace()) return;
                // ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                // auto klass =method->GetDeclaringClass();
                // //NewClass(klass);
                // std::string class_name = klass -> PrettyDescriptor();
                // std::string method_name(class_name + "::" + method->GetName());
                std::stringstream stream;
                if(osr)
                    method_name += "(osr)";
                LOG(WARNING)<< "Leakleak,JIT method: " << method_name << " and code: " << (void*)(code);
                
                stream<< "JIT method: " << method_name << " and code: " << (void*)(code);
                
                // LOG(WARNING)<< "Leakleak,JIT method: " << method_name << " (osr)and code: " << reinterpret_cast<uintptr_t>(code);
                my_lock2.lock();
                pc_method[reinterpret_cast<uintptr_t>(code)]=method_name;
                my_lock2.unlock();
            }

            void Leaktrace::new_obj(mirror::Object *obj,uint32_t _byte){
                if(!try_thread_istrace()) return;
                //obj->
                if(Runtime::Current()-> GetHeap()==nullptr){
                    return;
                }
                if(obj == nullptr ) return;
                find_new++;
                tol_size += _byte;
                if(_byte < (uint32_t)getTsize()) return;
                auto ptr = Runtime::Current()-> GetHeap()-> main_access_bitmap_.get();
                if(ptr==nullptr||!ptr->HasAddress(obj))return;
    
                uintptr_t pc = Thread::Current()-> GetAllocSite();
                Thread::Current()-> SetAllocSite();

                // {
                //    ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                //    NewClass(obj->GetClass(),(void*)pc);
                //      ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                //     std::string class_name = obj->GetClass() -> PrettyDescriptor();
                //      LOG(WARNING)<< "Leakleak,update class: " << class_name ;
                //     LOG(WARNING)<<"Leakleak,and class from "<<(void*)pc;
                // }

                if(pc != 0 ){
                    my_lock2.lock();
                    if(pc_class.count(pc)==0){
                        ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                        pc_class[pc] = obj->GetClass() -> PrettyDescriptor();
                        // LOG(WARNING)<< "Leakleak,alloc obj: " << pc_class[pc] <<" pc:"<<(void*)pc;
                    }
                    count_obj ++;
                    S_pc[pc]++;
                    size_t idx = get_obj_idx(obj);
                    pc_cnt[obj] = S_pc[pc];
                    if(_byte < large_size){
                        my_lock[idx].lock();
                        addr_pc[idx][obj]={pc,GC_K};
                        my_lock[idx].unlock();
                    }
                    else{
                        addr_large_pc[obj]={pc,GC_K};
                    }
                    my_lock2.unlock();
                }
                {
                    return;
                }
                //LOG(WARNING)<<"Leakleak,new obj "<<reinterpret_cast<uint64_t>((void*)obj)<<" pc is "<<pc;
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
        //             // ALOGD("Leakleak,need to check class: %s ",s.c_str());
        //             // for(auto c:_set2) ALOGD("Leakleak, string in set: %s ",c.c_str());
        //             int code = u->IdentityHashCode();
        //             t = clock() - t;
        //             code_time += t; 

        //             to_do[code]=u;
        //             // if(work(code,u)){
        //             //     // _set2.erase(_set2.find(s));
        //             // }
        //         }
            
        //         // ALOGD("Leakleak,addedge %s to %s",u->PrettyTypeOf().c_str(),v->PrettyTypeOf().c_str());
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
        //         // ALOGD("Leakleak, chian of ref is :  %s",ret.c_str());
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
        //         // ALOGD("Leakleak,ADD code: %d",x);
                
        //         _set.insert(x);
        //     }

        //     void addname(std::string x){
        //         // ALOGD("Leakleak,ADD name: %s",x.c_str());
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