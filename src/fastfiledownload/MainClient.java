package fastfiledownload;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Scanner;


/**
 * 
 * @author rkandur
 * 
 *         Main Client Class which holds the logic for register/login/exit with
 *         respect to registration server and also holds logic to deal with the
 *         interaction to DirectoryServers and also share files with other
 *         clients using FastFileDownload
 *
 */
public class MainClient {

	// registration server details
	public static HashMap<String, HashSet<String>> m_filesClients = new HashMap<String, HashSet<String>>();
	public static String m_regServerIp = getValueFromConfig("regServerIp");
	public static int m_regServerPort = Integer.parseInt(getValueFromConfig("regServerPort"));

	// directory server details
	public static String m_dirServerIp = new String();
	public static int m_dirServerPort = -1;
	public static Socket m_dirSocket = null;

	public static void main(String[] args) {

		// Deamon thread to handle fastfiledownload requests in a separate thread
		try {
			ServerSocket fastFileSocket = new ServerSocket(40000);
			new ClientDownloaderThread(fastFileSocket).start();
		} catch (IOException e) {
			System.exit(1);
		}

		try {

			Socket regSocket = new Socket(m_regServerIp, m_regServerPort);

			//continuously reads from the command line and delegates the client actions to helper me
			while(true){
				
				String command = (new Scanner(System.in)).nextLine().toLowerCase();
				
				DataInputStream inpStream = new DataInputStream(regSocket.getInputStream());
				DataOutputStream outStream = new DataOutputStream(regSocket.getOutputStream());
				
				executeClientCommand(inpStream, outStream, command);
				
			}

		} catch (UnknownHostException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	private static String getValueFromConfig(String key) {

		try {
			File file = new File("client.cfg");
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

	private static void executeClientCommand(DataInputStream in, DataOutputStream out, String input) {
		try {
			if(input.startsWith("share ")){
				handleShareCommand(in, out, input);
			}else if(input.startsWith("find ")){
				handleFindCommand(in, out, input);
			}else if(input.startsWith("remove ")){
				handleRemoveCommand(in, out, input);
			}else if(input.startsWith("servershare")){
				handleServerShareCommand(in, out, input);
			}else if(input.startsWith("fastfiledownload ")){
				handleFastFileDownloadCommand(in, out, input);
			} else if(input.startsWith("login ")) {
				handleLoginCommand(in, out, input);
			} else{
				// this should be the registration case or an invalid case that
				// is handled at server and the response is returned.
				out.writeUTF(input);
				String output = in.readUTF();
				System.out.println(output);
			} 
		}catch (IOException e) {
			e.printStackTrace();
		}
	}

	private static void handleLoginCommand(DataInputStream in,
			DataOutputStream out, String input) {

		try {
			out.writeUTF(input);
			String response = in.readUTF();
			System.out.println(response);
			if(!response.startsWith("Server Response: 200 ")) {
				System.out.println("Login Failed with the response " + response);
				return;
			}

			response = response.substring(response.indexOf('\n')+1);
			//fill the directory server details after login
			String[] directoryServer = response.split(":");
			m_dirServerIp = directoryServer[0];
			m_dirServerPort = Integer.parseInt(directoryServer[1]);	
			System.out.println("dirServerIp [" + m_dirServerIp + "] dirServerPort [" + m_dirServerPort + "]");

		} catch (IOException e) {
			e.printStackTrace();
		}

	}

	private static void handleShareCommand(DataInputStream in,
			DataOutputStream out, String input) {

		if(input.split(" ").length <= 1) {
			System.out.println("405 Invalid arguments.");
			return;
		}

		try {
			m_dirSocket = new Socket(m_dirServerIp, m_dirServerPort);
			DataOutputStream dout = new DataOutputStream(m_dirSocket.getOutputStream());
			DataInputStream din = new DataInputStream(m_dirSocket.getInputStream());
			dout.writeUTF(input);
			String output = din.readUTF();
			System.out.println(output);
			dout.close();
			din.close();
			m_dirSocket.close();
		} catch (IOException e1) {
			e1.printStackTrace();
		}

	}

	private static void handleFastFileDownloadCommand(DataInputStream in,
			DataOutputStream out, String input) {

		if(input.split(" ").length != 2) {
			System.out.println("405 Invalid arguments.");
			return;
		}

		String[] toks = input.split(" ");
		if(!m_filesClients.containsKey(toks[1])) {
			//to make sure that the list of clients with the file is already known
			System.out.println("Do a find first, then do a fastfiledownload of the file !!!");
			return;
		}

		int currentPart = 1;
		int noOfClients = m_filesClients.get(toks[1]).size();
		ArrayList<ClientReceiverThread> receiverThreadGroup = new ArrayList<ClientReceiverThread>();

		for(String s: m_filesClients.get(toks[1])) {
			
			String[] tokens = s.split(" ");
			Socket clientSocket;
			
			try {

				System.out.println("Connecting to client [" + s + " ] for downloading [" + currentPart + "] of [ " + toks[1] + "].");
				
				clientSocket = new Socket(tokens[0].split(":")[0], 40000);
				ClientReceiverThread t = new ClientReceiverThread(clientSocket, toks[1], noOfClients, currentPart++);
				t.start();
				receiverThreadGroup.add(t);
				
			} catch (UnknownHostException e1) {
				e1.printStackTrace();
			} catch (IOException e1) {
				e1.printStackTrace();
			}
			
		}

		// wait until all threads are done
		for(ClientReceiverThread t: receiverThreadGroup) {
			try {
				t.join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}

		try {
			
			currentPart = 1;
			
			File outputFile = new File(new StringBuffer().append("./shared_dir/").append("FFD_").append(toks[1]).toString() );
			
			FileOutputStream fileOutStream = new FileOutputStream(outputFile);
			BufferedOutputStream buffOutStream = new BufferedOutputStream(fileOutStream);

			for(int n=1; n <= noOfClients; ++n) {

				File file = new File(new StringBuffer().append("./shared_dir/").append("part").append(currentPart++).toString());

				FileInputStream fileInpStream = new FileInputStream(file);
				BufferedInputStream buffInpStream = new BufferedInputStream(fileInpStream);
				long totalLength = file.length();
				byte[] bytes = new byte[(int) totalLength];

				while ((buffInpStream.read(bytes)) > 0) {
					buffOutStream.write(bytes);
				}

				fileInpStream.close();
				buffInpStream.close();

			}
			
			buffOutStream.close();
			fileOutStream.close();

		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (UnsupportedEncodingException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}

	}

	private static void handleRemoveCommand(DataInputStream in, DataOutputStream out, String input) {

		if(input.split(" ").length <= 1) {
			System.out.println("405 Invalid arguments.");
			return;
		}

		try {
			m_dirSocket = new Socket(m_dirServerIp, m_dirServerPort);
			
			DataOutputStream dataOutStream = new DataOutputStream(m_dirSocket.getOutputStream());
			DataInputStream dataInpStream = new DataInputStream(m_dirSocket.getInputStream());
			
			dataOutStream.writeUTF(input);
			String output = dataInpStream.readUTF();
			System.out.println(output);
			
			dataInpStream.close();
			dataOutStream.close();
			m_dirSocket.close();
			
		} catch (IOException e) {
			e.printStackTrace();
		}

	}

	private static void handleFindCommand(DataInputStream in, DataOutputStream out, String input) {

		if(input.split(" ").length != 2) {
			System.out.println("405 Invalid arguments.");
			return;
		}

		try {

			m_dirSocket = new Socket(m_dirServerIp, m_dirServerPort);
			
			DataOutputStream dataOutStream = new DataOutputStream(m_dirSocket.getOutputStream());
			DataInputStream dataInpStream = new DataInputStream(m_dirSocket.getInputStream());
			dataOutStream.writeUTF(input);
			String output = dataInpStream.readUTF();
			System.out.println(output);

			boolean isFirstLine = true;
			String file = new String();
			HashSet<String> clients = new HashSet<String>();
			for(String l: output.split("\n")) {
				if(!isFirstLine) {
					String[] tokens = l.split(" ");
					clients.add(tokens[0]);
					file = tokens[1];
				}
				isFirstLine = false;
			}

			m_filesClients.put(file, clients);

			dataInpStream.close();
			dataOutStream.close();
			
			m_dirSocket.close();

		} catch (IOException e) {
			e.printStackTrace();
		}

	}

	private static void handleServerShareCommand(DataInputStream in,
			DataOutputStream out, String input) {

		if(input.split(" ").length != 1) {
			System.out.println("405 Invalid arguments.");
			return;
		}

		try {
			m_dirSocket = new Socket(m_dirServerIp, m_dirServerPort);
			
			DataOutputStream dataOutStream = new DataOutputStream(m_dirSocket.getOutputStream());
			DataInputStream dataInpStream = new DataInputStream(m_dirSocket.getInputStream());
			dataOutStream.writeUTF(input);
			String output = dataInpStream.readUTF();
			System.out.println(output);
			
			dataOutStream.close();
			dataInpStream.close();
			m_dirSocket.close();
			
		} catch (IOException e) {
			e.printStackTrace();
		}

	}

}
