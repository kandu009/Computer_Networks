package fastfiledownload;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;


/**
 * 
 * @author rkandur
 * 
 *         Main Directory Server which talks to Registration Server for handling
 *         its status, talks to other Directory Servers for serving clients
 *         requests Also talks to clients to give them details regarding clients
 *         hosting the files.
 *
 */
public class MainDirectoryServer {
	
	public static HashMap<String, HashSet<String>> m_clientVsFiles = new HashMap<String, HashSet<String>>();
	public static String m_regIpaddress = getValueFromConfig("regIpaddress");
	public static int m_regPort = Integer.parseInt(getValueFromConfig("regPort"));
	public static int m_dirPort = Integer.parseInt(getValueFromConfig("dirPort")); 
	
	public static void main(String[] args) {
		
		// directory server socket which responds to all other client requests
		ServerSocket serverSocket = null;
		 try {
			 serverSocket = new ServerSocket(m_dirPort);
		 } catch (IOException e) {
			 System.err.println("Could not listen on port: 10007.");
			 System.exit(1);
		 }
		
		try {
			// Registration thread which keeps registering itself with the registration server for every 30seconds.
			Socket regSocket = new Socket(m_regIpaddress, m_regPort);
			ScheduledThreadPoolExecutor s = new ScheduledThreadPoolExecutor(1);
			s.scheduleWithFixedDelay(new DirectoryRegistrationThread(regSocket), 0, 5000, TimeUnit.MILLISECONDS);
			
		} catch (UnknownHostException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		while(true) {
			
			 Socket clientSocket = null;
			 
			 try {
				 
				 clientSocket = serverSocket.accept();
				 DataOutputStream dataOutStream = new DataOutputStream(clientSocket.getOutputStream());
				 DataInputStream dataInpStream = new DataInputStream(clientSocket.getInputStream());
				 
				 handleClientRequest(dataInpStream, dataOutStream, clientSocket);
				 
			 } catch (IOException e) {
				 System.err.println("Accept failed.");
				 System.exit(1);
			 }
		}
		
	}

	private static String getValueFromConfig(String key) {

		try {
			File file = new File("server.cfg");
			FileReader fileReader = new FileReader(file);
			BufferedReader bufferedReader = new BufferedReader(fileReader);
			String line;
			while ((line = bufferedReader.readLine()) != null) {
				if(line.startsWith(key)) {
					String[] toks = line.split("=");
					if(toks.length != 2) {
						continue;
					}
					return toks[1];
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		return new String();
	}

	private static void handleClientRequest(DataInputStream in,
			DataOutputStream out, Socket clientSocket) {
		
		String input = new String();
		try {
			input = in.readUTF();
			System.out.println("Receieved " + input);
		} catch (IOException e1) {
			e1.printStackTrace();
		}

		if(input.startsWith("share ")){
			handleShareCommand(clientSocket, in, out, input);
		} else if(input.startsWith("find ")){
			handleFindCommand(in, out, input);
		} else if(input.startsWith("remove ")){
			handleRemoveCommand(clientSocket, in, out, input);
		} else if(input.startsWith("lookup ")) {
			handleLookupCommand(out, input);
		} else if(input.startsWith("filelist")) {
			handleFileListCommand(out, input);
		} else if(input.startsWith("servershare")){
			handleServerShareCommand(in, out, input);
		} else{
			try {
				// this is an ivalid case, throw out an error
				out.writeUTF("400 INVALID_COMMANDS");
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		
	}
	
	private static void handleFileListCommand(DataOutputStream out, String input) {
		
		if(input.split(" ").length != 1) {
			try {
				out.writeUTF("");
			} catch (IOException e) {
				e.printStackTrace();
			}
			return;
		}
		
		HashSet<String> files = new HashSet<String>();
		for(String s: m_clientVsFiles.keySet()) {
			files.addAll(m_clientVsFiles.get(s));
		}
		
		StringBuffer response = new StringBuffer();
		for(String f: files) {
			response.append(f).append(" ");
		}
		
		try {
			out.writeUTF(response.toString());
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}

	private static void handleLookupCommand(DataOutputStream out, String input) {

		if(input.split(" ").length != 2) {
			try {
				out.writeUTF("");
			} catch (IOException e) {
				e.printStackTrace();
			}
			return;
		}
		
		String[] reqToks = input.split(" ");
		StringBuffer response = new StringBuffer();
		
		for(String s: m_clientVsFiles.keySet()) {
			if(m_clientVsFiles.get(s).contains(reqToks[1])) {
				response.append(s).append(" ");
			}
		}
		
		try {
			out.writeUTF(response.toString());
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}

	private static void handleShareCommand(Socket clientSocket, DataInputStream in,
			DataOutputStream out, String input) {

		if(input.split(" ").length <= 1) {
			try {
				out.writeUTF("405 Invalid arguments.");
			} catch (IOException e) {
				e.printStackTrace();
			}
			return;
		}
		
		try {

			StringBuffer response = new StringBuffer();
			response.append("200 SUCCESS").append("\n");

			String[] reqToks = input.split(" ");
			HashSet<String> hs = new HashSet<String>();
			
			for(int i = 1; i < reqToks.length; ++i) {
				hs.add(reqToks[i]);
				response.append(reqToks[i]).append("\n");
				System.out.println(reqToks[i]);
			}
			
			//get the client IP and port for maintaining our local store
			StringBuffer s = new StringBuffer();
			s.append(clientSocket.getInetAddress().toString().substring(1)).append(":").append("40000");
			m_clientVsFiles.put(s.toString(), hs);
			
			for(String s1: m_clientVsFiles.keySet()) {
				System.out.println(s);
				System.out.println(m_clientVsFiles.get(s1));
			}
			
			out.writeUTF(response.toString());
			System.out.println(response.toString());
			
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}
	
	private static void handleRemoveCommand(Socket clientSocket, DataInputStream in, DataOutputStream out, String input) {
		
		if(input.split(" ").length <= 1) {
			try {
				out.writeUTF("405 Invalid arguments.");
			} catch (IOException e) {
				e.printStackTrace();
			}
			return;
		}
		
		try {
			
			//get the client IP and port for maintaining our local store
			StringBuffer s = new StringBuffer();
			s.append(clientSocket.getInetAddress().toString().substring(1)).append(":").append("40000");
			
			StringBuffer output = new StringBuffer();
			
			if(!m_clientVsFiles.containsKey(s.toString())) {
				
				output.append("410 CLIENT_NOT_FOUND");
				out.writeUTF(output.toString());
				return;
				
			} else {
				
				HashSet<String> h = m_clientVsFiles.get(s.toString());
				String[] reqToks = input.split(" ");
				
				if(!h.contains(reqToks[1])) {
					output.append("404 FILE_NOT_FOUND");
					out.writeUTF(output.toString());
					return;
				}
				
				// remove the file from clients list and update the records
				h.remove(reqToks[1]);
				m_clientVsFiles.put(s.toString(), h);
				out.writeUTF("200 SUCCESS");
				
				return;
				
			}
			
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}

	private static void handleFindCommand(DataInputStream in, DataOutputStream out, String input) {

		if(input.split(" ").length != 2) {
			System.out.println("405 Invalid arguments.");
			return;
		}
		
		System.out.println(input);
		HashSet<String> clients = new HashSet<String>();
		String[] reqToks = input.split(" ");
		
		// first search for the file locally
		for(String s: m_clientVsFiles.keySet()) {
			if(m_clientVsFiles.get(s).contains(reqToks[1])) {
				clients.add(s);
			}
		}
		
		System.out.println("Finished local lookup");
		
		// query register server for rest of the directory servers
		HashSet<String> servers = new HashSet<String>();
		
		try {
			
			Socket regSocket = new Socket(m_regIpaddress, m_regPort);
			DataInputStream dataInpStream = new DataInputStream(regSocket.getInputStream());
			DataOutputStream dataOutStream = new DataOutputStream(regSocket.getOutputStream());
			dataOutStream.writeUTF("list");
			
			String response = dataInpStream.readUTF();
			System.out.println("Response" + response);
			for(String s: response.split(" ")) {
				if(!s.isEmpty()) {
					servers.add(s);
				}
			}
			
			regSocket.close();
			dataInpStream.close();
			dataOutStream.close();
			
		} catch (UnknownHostException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		// query each directory server for the list of clients
		for(String server: servers) {
			
			try {
			
				System.out.println("Making connection to " + server);
				String[] tokens = server.split(":");
				
				if(tokens.length != 2) {
					continue;
				}
				
				Socket s = new Socket(tokens[0], Integer.parseInt(tokens[1]));
				DataInputStream dataInpStream = new DataInputStream(s.getInputStream());
				DataOutputStream dataOutStream = new DataOutputStream(s.getOutputStream());
				dataOutStream.writeUTF("lookup " + reqToks[1]);
				
				String response = dataInpStream.readUTF();
				for(String str: response.split(" ")) {
					if(!str.isEmpty()) { 
						clients.add(str);
					}
				}
				
				s.close();
				dataInpStream.close();
				dataOutStream.close();
				
			} catch (UnknownHostException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		
		// aggregate all clients info and send response
		StringBuffer response = new StringBuffer();
		response.append("200 SUCCESS").append("\n");
		
		System.out.println("Aggregating");
		for(String c: clients) {
			if(!c.isEmpty()) {
				response.append(c).append(" ").append(reqToks[1]).append("\n");
			}
		}
		
		System.out.println("Output" + response);
		try {
			out.writeUTF(response.toString());
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	private static void handleServerShareCommand(DataInputStream in, DataOutputStream out, String input) {

		if(input.split(" ").length != 1) {
			try {
				out.writeUTF("405 Invalid arguments.");
			} catch (IOException e) {
				e.printStackTrace();
			}
			return;
		}
		
		HashSet<String> files = new HashSet<String>();

		// first get all the files available locally
		for(String s: m_clientVsFiles.keySet()) {
			files.addAll(m_clientVsFiles.get(s));
		}		
		
		// query register server for rest of the directory servers
		HashSet<String> servers = new HashSet<String>();
		try {
			Socket regSocket = new Socket(m_regIpaddress, m_regPort);
			DataInputStream dataInpStream = new DataInputStream(regSocket.getInputStream());
			DataOutputStream dataOutStream = new DataOutputStream(regSocket.getOutputStream());
			dataOutStream.writeUTF("list");
			
			String response = dataInpStream.readUTF();
			for(String s: response.split(" ")) {
				servers.add(s);
			}
			
			regSocket.close();
			dataInpStream.close();
			dataOutStream.close();
			
		} catch (UnknownHostException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		// query each directory server for the list of files
		for(String server: servers) {
			
			try {
				
				String[] tokens = server.split(":");
				if(tokens.length != 2) {
					continue;
				}
				
				Socket s = new Socket(tokens[0], Integer.parseInt(tokens[1]));
				DataInputStream dataInpStream = new DataInputStream(s.getInputStream());
				DataOutputStream dataOutStream = new DataOutputStream(s.getOutputStream());
				dataOutStream.writeUTF("filelist");
				
				String response = dataInpStream.readUTF();
				for(String str: response.split(" ")) {
					files.add(str);
				}
				
				s.close();
				dataInpStream.close();
				dataOutStream.close();
				
			} catch (UnknownHostException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
			}
			
		}
		
		// aggregate all files info and send response
		StringBuffer response = new StringBuffer();
		response.append("200 SUCCESS").append("\n");
		for(String f: files) {
			response.append(f).append("\n");
		}
		
		try {
			out.writeUTF(response.toString());
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}

}
