package fastfiledownload;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Random;
import java.util.Set;


/**
 * 
 * @author rkandur Helper class which has the common utility methods and logic
 *
 */
public class RegistrationServerHelper {

	private static ArrayList<String> m_directoryServers = new ArrayList<String>();
	public static HashMap<String, RegistrationServerTimer> m_validDirectoryServers  = new HashMap<String, RegistrationServerTimer>(); 
	public static HashMap<String, String> m_activeClients = new HashMap<String, String>();
	private static HashMap<String, Integer> m_activeDirectoryServers = new HashMap<String, Integer>();

	public static ArrayList<String> getServerList() {

		m_directoryServers.clear();
		
		Set<String> servers = m_validDirectoryServers.keySet();
		for(String s : servers){
			if(m_validDirectoryServers.get(s).isValid())
				m_directoryServers.add(s);
		}
		
		return m_directoryServers;
	}

	public static ArrayList<String> getListofServers(){
		return m_directoryServers;
	}

	public static String getResponseFromCode(int errorCode){
		
		String response = null;
		
		switch(errorCode){
			case 200: 
			{
				response = "200 SUCCESS";
				break;
			}
			case 400:
			{
				response = "400 INVALID COMMANDS";
				break;
			}
			case 401:
			{
				response = "401 USER ALREADY EXISTS";
				break;
			}
			case 402:
			{
				response = "402 PASSWORDS DO NOT MATCH";
				break;
			}
			case 403:
			{
				response = "403 INVALID USERNAME/PASSWORD";
				break;
			}
			case 404:
			{
				response = "404 FILE NOT FOUND";
				break;
			}
			case 405:
			{
				response = "405 INVALID ARGUMENTS";
				break;
			}
			case 406:
			{
				response ="406 USER NOT LOGGED IN";
				break;
			}
			case 409:
			{
				response ="USER ALREADY LOGGED IN";
				break;
			}
			default:
			{
				response = "400 INVALID COMMANDS";
				break;
			}
		}
		return response;

	}

	public static String getDirectoryServerforClient() {
		
		ArrayList<String> servers = getServerList();
		if(servers.size()==0){
			return "No Active Servers, Please wait and try again !!!";
		}else{
			/*int randomNumber = (new Random()).nextInt(servers.size());
			return servers.get(randomNumber);*/
			return getServerForClientAccordingToLoad(servers);
		}
	}
	
	private static String getServerForClientAccordingToLoad(ArrayList<String> servers) {
		
		// if there are no actively serving servers, add one
		if(m_activeDirectoryServers.size() == 0){
			m_activeDirectoryServers.put(servers.get(0), 1);
			return servers.get(0);
		}

		// If any of the servers is free, assign the client to it
		for(String s :  servers){
			if(!m_activeDirectoryServers.containsKey(s)){
				m_activeDirectoryServers.put(s, 1);
				return s;
			}
		}
		
		// If all servers are handling atleast one client, then assign client to
		// the lowest loaded server
		int i=1;
		while(i<10){
			Set<String> keys = m_activeDirectoryServers.keySet();
			for(String k:keys){		
				if(m_activeDirectoryServers.get(k).equals(i) && servers.contains(k)){
					m_activeDirectoryServers.put(k, i+1);
					return k;
				}
			}
			i=i+1;
		}

		// ideally this should not be used. If all others fail, randomly allocate servers
		int randomNumber = (new Random()).nextInt(servers.size());
		return servers.get(randomNumber);

	}
	
}
