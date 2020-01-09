
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

            int mod_k = 16;
            int need_log =0 ;
            std::mutex my_lock;
            std::mutex my_lock2;
            std::set<uint64_t> S_pc;
            std::map<uintptr_t,std::string> pc_method;
            int count_obj = 0;
            int tol_size =0;
            int find_new=0;
            
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
            void Leaktrace::set_bitmap_ptr(uint64_t _end){
                bitmap_ptr=_end;
            };
            uint64_t Leaktrace::get_main_begin(){
                return main_begin;
            };
            uint64_t Leaktrace::get_main_end(){
                return main_end;
            };
            uint64_t Leaktrace::get_bitmap_ptr(){
                return bitmap_ptr;
            };
            //std::unordered_map<uint64_t,uint64_t> addr_pc,addr_pc_t;

            void Leaktrace::dump_info(uint64_t tim){
                if(!try_thread_istrace()) return;
                if(need_log)LOG(WARNING)<<"ZHANG,info about gc time: "<<tim ;
                // std::fstream F;
                // F.open("/data/local/tmp/track");
                // F<<cnt++<<" hello_world\n";
                // F.close();
                // ALOGD("ZHANG,THIS IS LOG");
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



            void Leaktrace::addedge(mirror::Object *obj){
                
                int gc = Thread::Current()-> GetGCnum();
                if(!try_thread_istrace() || gc % mod_k != 0 ){
                    // my_lock.unlock();
                    return;
                }
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                // int t = clock();
                // // if(strstr(name,"myapp")!=NULL)
                // objcnt++;
                // finder.addedge(ref,obj);
                // t = clock() - t;
                // map_time += t;
                 if(Runtime::Current()-> GetHeap()==nullptr){
                    // my_lock.unlock();
                    return;
                }
                uint64_t o = reinterpret_cast<uint64_t>((void*)obj);
                if(o<main_begin||o>=main_end){
                    // my_lock.unlock();
                    return;
                }
                //ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                std::pair<uint64_t,uint16_t> p={0,0};
                my_lock.lock();
                if(addr_pc.count(low(obj))){
                    p=addr_pc[low(obj)];
                    my_lock.unlock();
                }
                else{
                    my_lock.unlock();
                    return;
                }
                
                auto ptr = Runtime::Current()-> GetHeap()-> main_access_bitmap_ .get();
                if(p.first != 0 && ptr != nullptr){
                        my_lock.lock();
                        if(ptr ->Test(obj)){
                            // addr_pc_t[o]=p;
                            ptr ->Clear(obj);
                            vis[low(obj)]=p.second;
                        //this obj is not touch during k gc
                        //LOG(WARNING)<<"ZHANG,this "<<reinterpret_cast<uint64_t>((void*)obj)<<" NOTLEAK "<<
                        //     "and alloc at"<<p;
                        }

                        else if(ptr ->Test(obj) == 0 && vis.count(low(obj)) == 0 && gc - p.second >= mod_k){
                        /* code */
                            //s_ans.insert(p);
                            ++m_ans[p.first];
                            vis[low(obj)]=p.second;
                           // LOG(WARNING)<<"ZHANG,this "<<o<<" LEAK and alloc at"<<p;
                        }
                        my_lock.unlock();
                }
                {
                    return;
                }
            }

            void Leaktrace::dump_str(std::string S){
                // return;
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                // if(strstr(name,"myapp")!=NULL)
                ALOGD("ZHANG, %s ",S.c_str());

            }

            void Leaktrace::dump_init(){
                if(!try_thread_istrace()) return;
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
            int Leaktrace::try_thread_istrace(){
                if(Thread::Current() == nullptr){
                    // return 0;
                    char name[LEN] = "noname";
                    GetProcNameByPid(name, LEN, getpid());
                    
                    std::vector<std::string> v = io.get_app();
                    int f = 0;
                    for(auto str: v){
                        if(strstr(name,str.c_str())!=NULL){
                            f=1;
                        }
                    }
                    if(f){
                        // is = 1;
                        ALOGD("ZHANG, heap_BEGIN no thread,and app name is trace:%s   ",name);
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
                        return 1;
                    }
                    else
                    {
                        // is = 0;
                        // Thread::Current()-> Setistrace(0);
                        ALOGD("ZHANG, heap_BEGIN no thread,but this app is not to trace:%s",name);
                        /* code */
                        return 0;
                    }
                }
                int is = Thread::Current()-> Getistrace();
                if(is == -1){
                    // return 0;
                    // std::string t<"ZHANG,hhread_name;
                    // Thread::Current() ->GetThreadName(thread_name);
                    // LOG(WARNING)<ave a try of ThreadName:"<<thread_name;

                    char name[LEN] = "noname";
                    GetProcNameByPid(name, LEN, getpid());
                    
                    std::vector<std::string> v = io.get_app();
                    int f = 0;
                    for(auto str: v){
                        if(strstr(name,str.c_str())!=NULL){
                            f=1;
                        }
                    }
                    if(f){
                        is = 1;
                        ALOGD("ZHANG, Thread_BEGIN ,and app name is trace:%s   ",name);
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
                        Thread::Current()-> Setistrace(1);
                    }
                    else
                    {
                        is = 0;
                        Thread::Current()-> Setistrace(0);
                        // ALOGD("ZHANG, Thread_BEGIN ,but this app is not to trace:%s",name);
                        /* code */
                    }
                }
                return is;
            }
            
            void Leaktrace::heap_init(){
                if(try_thread_istrace())
                    addr_pc.clear();
                // char name[LEN] = "noname";
                // GetProcNameByPid(name, LEN, getpid());
                
                // std::string str = io.get_app();
                // if(strstr(name,str.c_str())!=NULL){
                //     ALOGD("ZHANG, heap_BEGIN ,and app name is trace:%s   ",name);
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
                //     ALOGD("ZHANG, heap_BEGIN ,but this app is not to trace:%s",name);
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
                //     ALOGD("ZHANG, LOG ,and app name is trace:%s",name);
                //     istrace = true; 
                //     // addr_pc.clear();
                // }
                // else{
                //     //ALOGD("ZHANG, LOG ,but this app is not to trace:%s",name);
                //     /* code */
                
                // }

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
                if(!try_thread_istrace()) return;
                // ALOGD("ZHANG, GC_BEGIN and this is %d",GC_K+1);
                GC_K++;
                Thread::Current()-> SetGCnum(GC_K);
                // if(GC_K % mod_k == 0)
                //     gc_time = clock();
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

            void Leaktrace::gc_end(){
                // GetProcNameByPid(name, LEN, getpid());
                if(!try_thread_istrace()) return;
                int gc = Thread::Current()-> GetGCnum();
                if(gc % mod_k == 0){
                // char name[LEN] = "noname";
                    // gc_time = clock() - gc_time;
                    //TODO:
                    /*dump something */
                    // ALOGD("ZHANG, GC_END and this is %d",gc);
                    // swap(addr_pc_t,addr_pc);
                    // addr_pc_t.clear();
                    //ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                    // auto ptr = Runtime::Current()-> GetHeap()-> main_access_bitmap_ .get();
                    // for(auto it:addr_pc){
                    //     if(ptr->Test(it.first)){
                    //         ptr->Clear(it.first);
                    //     }
                    //     else{
                    //         LOG(WARNING)<<"ZHANG,this "<<it.first<<" LEAK and alloc at"<<it.second<<"and GC is "<<gc;
                    //     }
                    // }
                    char name[LEN] = "noname";
                    GetProcNameByPid(name, LEN, getpid());
                    std::stringstream stream;
                    LOG(WARNING)<<"ZHANG,info about: "<<name <<" hashtable_size:"<<16* (addr_pc.size()+vis.size());
                    stream <<"info about: "<<name <<":\n";
                    my_lock.lock();
                    for(auto it:m_ans){
                            auto pc = it.first;
                            jit::Jit* jit = Runtime::Current()->GetJit();
                            if(pc > 0 && (jit!=nullptr && jit->GetCodeCache()->ContainsPc((void*)pc))){
                                my_lock2.lock();
                                auto jt = pc_method.upper_bound(it.first);
                                my_lock2.unlock();
                                if(jt == pc_method.begin()) continue;
                                jt--;
                                // if(it.second>1)
                                    LOG(WARNING)<<"ZHANG,maybe leak at "<<(void*)it.first<<"and GC is " << gc << " and from method"<< jt->second;
                                    stream <<"maybe leak at "<<(void*)it.first<<"and GC is " << gc << " and from method"<< jt->second <<std::endl;

                            }
                            else{
                                // if(it.second>1)
                                    LOG(WARNING)<<"ZHANG,maybe leak at  "<<(void*)it.first<<"and GC is " << gc ;
                                    stream<<"maybe leak at  "<<(void*)it.first<<"and GC is " << gc <<std::endl;
                            }
                    }
                    std::vector<uint32_t> to_erase;
                    std::vector<uint32_t> to_erasev;
                    for(auto it:vis){
                        if(GC_K - it.second >= mod_k*3/2)
                            to_erasev.push_back(it.first);
                    }
                    for(auto it:addr_pc){
                        if(GC_K - it.second.second >= mod_k*3/2 && vis.count(it.first)==0)
                            to_erase.push_back(it.first);
                    }
                    for(auto it:to_erase){
                        addr_pc.erase(it);
                    }
                    for(auto it:to_erasev){
                        vis.erase(it);
                    }
                    m_ans.clear();
                    my_lock.unlock();
                    LOG(WARNING)<<"ZHANG,alloc obj tol:"<<count_obj;
                    LOG(WARNING)<<"ZHANG,alloc obj byte tol:"<<tol_size;
                    LOG(WARNING)<<"ZHANG,diff pc tol:"<<S_pc.size();
                    LOG(WARNING)<<"ZHANG,find new tol:"<<find_new;

                    stream<<"alloc obj tol:"<<count_obj<<std::endl;
                    stream<<"alloc obj byte tol:"<<tol_size<<std::endl;
                    stream<<"diff pc tol:"<<S_pc.size()<<std::endl;
                    stream<<"find new tol:"<<find_new<<std::endl;
                    io.put_ans(stream.str());
                    // LOG(WARNING)<<"ZHANG,cost mem:"<<sizeof(getInstance())+sizeof(*Runtime::Current()-> GetHeap()-> main_access_bitmap_ .get());
                    // if((gc/2) % mod_k == 0)vis.clear();
                }
                
                {
                    return;
                }
                // gc_time = clock() - gc_time;
                // istrace = false; 
                // // ALOGD("ZHANG, GC_END,read_time: %d, GC_time: %d, map_time: %d ,name: %d ,code: %d!,objcnt: %d",read_time,gc_time,map_time,name_time,code_time,finder.getsize());
                // finder.work();
                // finder.clear();
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
                    //ALOGD("ZHANG, %s touch %d",S.c_str(),(int)i);
                }
            
            }

            void Leaktrace::CC_move_from_to(mirror::Object *I,mirror::Object *J){
                if(!try_thread_istrace()) return;
                if(I == nullptr||J == nullptr||I == J) return;
                if(Runtime::Current()-> GetHeap()==nullptr) return;
                //ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                auto ptr = Runtime::Current()-> GetHeap()-> main_access_bitmap_.get();
                if(ptr!=nullptr&&ptr->HasAddress(I)&&ptr->HasAddress(J)){
                    my_lock.lock();
                        if(addr_pc.count(low(I))){
                            addr_pc[low(J)]=addr_pc[low(I)];
                            addr_pc.erase(low(I));
                                //addr_pc[reinterpret_cast<uint64_t>((void*)I)]=0;
                            if(ptr->Test(I)){
                                ptr -> Set(J);
                                ptr -> Clear(I);
                            }
                            if(vis.count(low(I))){
                                vis[low(J)]=vis[low(I)];
                                vis.erase(low(I));
                            }
                            my_lock.unlock();
                            return;
                        }
                    my_lock.unlock();
                }
                {
                    // my_lock.unlock();
                    return;
                }
            }

            
            //std::set<mirror::Class *> classes_;
            void Leaktrace::NewClass(mirror::Class *klass) {
                if (klass && classes_.find(klass) == classes_.end()) {
                    classes_.insert(klass);

                    ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                    std::string class_name = klass -> PrettyDescriptor();
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
                    auto pointer_size = Runtime::Current()->GetClassLinker()->GetImagePointerSize();
                    auto direct_methods = klass->GetDirectMethods(pointer_size);
                    for (auto& m : direct_methods) {
                        NewMethodLinked(class_name, m);
                    }
                    auto virtual_methods = klass->GetVirtualMethods(pointer_size);
                    for (auto& m : virtual_methods) {
                        NewMethodLinked(class_name, m);
                    }
                    for (auto i = 0; i < klass->GetVTableLength(); i++) {
                        NewMethodLinked(class_name, *(klass->GetVTableEntry(i, pointer_size)));
                    }
                    // auto im_methods = klass->GetImTable();
                    // if (im_methods)
                    //   for (int i = 0; i < im_methods->GetLength(); ++i)
                    //     NewMethodLinked(class_name, im_methods->Get(i));
                    // DumpMethods(im_methods);

                    // interface table is an array of pair (Class*, ObjectArray<ArtMethod*>)
                    auto if_table = klass->GetIfTable();
                    auto method_array_size = klass->GetIfTableCount();
                    for (int i = 0; i < method_array_size; ++i) {
                        auto* method_array = if_table->GetMethodArray(i);
                        for (size_t k = 0; k < if_table->GetMethodArrayCount(i); k++) {
                            NewMethodLinked(class_name, *(method_array->GetElementPtrSize<ArtMethod*>(k, pointer_size)));
                        }
                    }
                }
            }




            void Leaktrace::NewMethodLinked(const std::string& class_name, ArtMethod &method) {
                // leading character 'm' means this line is Method information
                auto code = reinterpret_cast<uintptr_t>(method.GetEntryPointFromQuickCompiledCode());
                ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                std::string method_name(class_name + "::" + method.GetName());
                LOG(WARNING)<< "ZHANG,find class method: " << method_name << "and code: " << code;
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
                    LOG(WARNING)<< "ZHANG,update method: " << method_name << "and code: " << reinterpret_cast<uintptr_t>(code);
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
                if(need_log)LOG(WARNING)<< "ZHANG,JIT method: " << method_name << " and code: " << (void*)(code);
                
                stream<< "JIT method: " << method_name << " and code: " << (void*)(code);
                
                // else
                // LOG(WARNING)<< "ZHANG,JIT method: " << method_name << " (osr)and code: " << reinterpret_cast<uintptr_t>(code);
                my_lock2.lock();
                pc_method[reinterpret_cast<uintptr_t>(code)]=method_name;
                my_lock2.unlock();
            }
            // void Leaktrace::JITMethod(ArtMethod &method,uint32_t code) {
            //     // leading character 'm' means this line is Method information
            //     // auto code = reinterpret_cast<uintptr_t>(method.GetEntryPointFromQuickCompiledCode());
            //     ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
            //     auto klass =method.GetDeclaringClass();
            //     std::string method_name(class_name + "::" + method.GetName());
            //     LOG(WARNING)<< "ZHANG,find class method: " << method_name << "and code: " << code;
            //     // PRINT(class_fp_, "m %8X %8X %s\n", code, code + method->GetCodeSize(), method_name.c_str());
            //     // method_meta_info_.push_back(ClassOrMethod(reinterpret_cast<unsigned int>(code),
            //     //                                         reinterpret_cast<unsigned int>(code) + method.GetCodeSize(),
            //     //                                         method_name));
            // }
            void Leaktrace::new_obj(mirror::Object *obj,int _byte){
                if(!try_thread_istrace()) return;
                //obj->
                if(obj == nullptr ) return;
                uint64_t o = reinterpret_cast<uint64_t>((void*)obj);

                find_new++;
                tol_size += _byte;
                if(_byte < getTsize()) return;
                if(o<main_begin||o>=main_end) return;
                uintptr_t pc = Thread::Current()-> GetAllocSite();
                //jit::Jit* jit = Runtime::Current()->GetJit();
                //if(pc > 0 && (jit!=nullptr && jit->GetCodeCache()->ContainsPc((void*)pc))){
                if(pc != 0 ){
                    // {
                    //     ReaderMutexLock mu(Thread::Current(), *Locks::mutator_lock_);
                    //     NewClass(obj->GetClass());
                    // }
                    S_pc.insert(pc);
                    count_obj ++;
                    my_lock.lock();
                    addr_pc[low(obj)]={pc,GC_K};
                    my_lock.unlock();
                }
                {
                    return;
                }
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