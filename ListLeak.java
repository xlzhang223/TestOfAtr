import java.util.*;
public class ListLeak
{
	int id;
	int[] something;
	void set_id(int x){
		id = x;
	}
	ListLeak(){
		id = 1;
		something = new int[1000];
	}
	void touch(){
		set_id(1);
		something[0]=1;
	}
	int make_leak(int no){
		int j=no;
		for(int i = 0; i < 5000; i++){
			j+=i;		
		}
		end.next = new ListLeak();
		end = end.next;
		return j;
	}
	int wait_for(int k,ListLeak t){
		int j=k;
		id = k;
		for(int i = 0; i < 5000; i++){
			touch();
			t.touch();
		}	
		return j+k;	
	}
	ListLeak next;
	ListLeak end;
	public static void main( String[] args ){
		int N = 1000000;	
		ListLeak o =  new ListLeak();
		o.end = o;
		for(int i = 0; i < 50; i++)o.make_leak(i*i);
		for(int i = 0; i < N; i++){
			ListLeak q =  new ListLeak();
			int f = o.wait_for(i,q);		
			if(i%10000==0)System.out.println("add List: "+i+" deop:"+f);
		}
	}
}
