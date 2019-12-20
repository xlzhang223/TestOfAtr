import java.util.List;
import java.util.LinkedList;
import java.math.*;
import java.util.*;
public class ListLeak
{
	
	String name;
	int id;
	int[] something;
	
	void set_id(int x){
		id = x;
	}
	ListLeak(){
		name = "zhang";
		id = 1;
		something = new int[100];
	}
	static List<ListLeak> list ;
	public static void main( String[] args ){
		list = new ArrayList<ListLeak>();
		int N = 300000;	
		ListLeak o =  new ListLeak();
		list.add(o);
		for(int i = 1; i< N; i++){
			ListLeak p =  new ListLeak();
			list.add(p);
		}
		for(int i = 0; i < 10; i++){
			for(int k = 1; k < N; k++){
				list.get(k).set_id(k);
			}
			for(int k =0; k < N/5; k++){
				ListLeak q =  new ListLeak();
			}			
			System.out.println("add List: "+i);
		}
	}
}
