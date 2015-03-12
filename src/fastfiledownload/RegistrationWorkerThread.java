package fastfiledownload;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.HashMap;

/**
 * 
 * @author rkandur
 * 
 *         Registration Server Deamon which serves all the registration requests
 *         which continuously come for every 30seconds from Directory Servers.
 *
 */
public class RegistrationWorkerThread extends Thread{
	
	Socket m_socket;
	boolean m_isUserLoggedIn;
	
	private static int MIN_PORT = 1024;
	private static int MAX_PORT = 65536;

	public RegistrationWorkerThread(Socket socket) {
		m_socket = socket;
	}

	public void run() {
		
		super.run();				

		try{

			DataInputStream dataInpStream = new DataInputStream(m_socket.getInputStream());
			String command = dataInpStream.readUTF();			
			DataOutputStream dataOutStream = new DataOutputStream(m_socket.getOutputStream());
			
			int responseCode = executeCommand(command);
			String response = "Server Response: " +RegistrationServerHelper.getResponseFromCode(responseCode);
			
			if(command.startsWith("addMe") && command.split(" ").length ==3){				
				
				if(responseCode ==ServerResponseCodes.ERR_200){
					
					for(String s: RegistrationServerHelper.getServerList()){
						response= response +"\n"+s;
					}

					dataOutStream.writeUTF(response);
					dataInpStream.close();
					dataOutStream.close();
					
				}
				
				dataOutStream.writeUTF(response);
				dataInpStream.close();
				dataOutStream.close();
				
			} else if(command.startsWith("list")) {
				
				// this should list out all the directory servers registered with the registration server
				StringBuffer resp = new StringBuffer();
				
				for(String s : RegistrationServerHelper.getServerList()) {
					if(!m_socket.getInetAddress().toString().startsWith("/"+s.substring(0, s.indexOf(":")))) {
						resp.append(s).append(" ");
					}
				}
				
				dataOutStream.writeUTF(resp.toString());
				dataInpStream.close();
				dataOutStream.close();
				
			} else{
				
				while(true){

					if(command.startsWith("login") && responseCode ==ServerResponseCodes.ERR_200) {
						response= response + "\n" + RegistrationServerHelper.getDirectoryServerforClient();						
					}
					
					dataOutStream.writeUTF(response);
					command = dataInpStream.readUTF();

					responseCode = executeCommand(command);
					response = "Server Response: " +RegistrationServerHelper.getResponseFromCode(responseCode);
					
				}
			}
		} catch (IOException e){

		}
	}


	private int executeCommand(String command) {
		
		HashMap<String, String> activeClients = RegistrationServerHelper.m_activeClients;
		
		//registration server request
		if(command.startsWith("addMe")){
			
			String[] cmdToks = command.split(" ");
			if(cmdToks.length != 4 && cmdToks.length !=3) {
				return ServerResponseCodes.ERR_400;
			}
			
			if(cmdToks[0].equals("addMe") && (cmdToks[1].split("\\.")).length == 4){
				
				int port = Integer.parseInt(cmdToks[2]);
				if(port > MIN_PORT && port < MAX_PORT) {
					// if chosen in not permitted range, report an error
					String serverDetails = cmdToks[1]+":" +cmdToks[2];
					if(RegistrationServerHelper.m_validDirectoryServers.containsKey(serverDetails)){
						RegistrationServerHelper.m_validDirectoryServers.remove(serverDetails);
					}
					RegistrationServerHelper.m_validDirectoryServers.put(serverDetails, new RegistrationServerTimer());
					return ServerResponseCodes.ERR_200;
					
				} else {
					return ServerResponseCodes.ERR_405;
				}
				
			} else {
				return ServerResponseCodes.ERR_400;
			}
			
		} else if(command.startsWith("register")){
			// register a new client
			
			String[] cmdToks = command.split(" ");
			if(cmdToks.length != 4) {
				return ServerResponseCodes.ERR_400;
			}
			
			if((!activeClients.containsKey(cmdToks[1])) && cmdToks[2].equals(cmdToks[3]) ) {
				activeClients.put(cmdToks[1], cmdToks[2]);
				return ServerResponseCodes.ERR_200;
			} else if(activeClients.containsKey(cmdToks[1])) {
				return ServerResponseCodes.ERR_401;
			} else if(!cmdToks[2].equals(cmdToks[3])) {
				return ServerResponseCodes.ERR_402;
			}
			
			return ServerResponseCodes.ERR_400;

		}else if(command.startsWith("login")){

			// serve a new client login request
			String[] cmdToks = command.split(" ");
			if(cmdToks.length != 3) {
				return ServerResponseCodes.ERR_405;
			}
			
			if(activeClients.containsKey(cmdToks[1])) {
				
				if(cmdToks[2].equals(activeClients.get(cmdToks[1]))) {
					
					if(m_isUserLoggedIn) {
						return ServerResponseCodes.ERR_409;
					}
					m_isUserLoggedIn = true;
					return ServerResponseCodes.ERR_200;
				} else {
					return ServerResponseCodes.ERR_403;
				}
				
			} else {
				return ServerResponseCodes.ERR_403;
			}

		}else if(command.equals("exit")){
			
			if(m_isUserLoggedIn){
				m_isUserLoggedIn = false;
				return ServerResponseCodes.ERR_200;
			}else {
				return ServerResponseCodes.ERR_406;
			}
			
		}
		return 0;

	}

}