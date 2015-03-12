package fastfiledownload;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;


/**
 * 
 * @author rkandur
 * 
 *         Registration Server which handles the clients registration/login and
 *         Directory servers registration
 *
 */
public class MainRegistrationServer {
	
	public static void main(String[] args) {
		
		try {
			
			ServerSocket server = new ServerSocket(getPortFromConfig());
			while(true){
				// continuously serve the requests
				Socket conn = server.accept();
				(new RegistrationWorkerThread(conn)).start();
			}

		} catch (IOException e) {			
			e.printStackTrace();
		}

	}

	private static int getPortFromConfig(){

		try {
			
			File file = new File("server.cfg");
			FileReader fileReader = new FileReader(file);
			BufferedReader bufferedReader = new BufferedReader(fileReader);
			StringBuffer sb = new StringBuffer();
			
			String line;
			while ((line = bufferedReader.readLine()) != null) {
				sb.append(line);
				sb.append("\n");
			}
			
			fileReader.close();			
			String servercfg = sb.toString();
			String[] arr = servercfg.split(":");
			String substring  = arr[1].substring(0, arr[1].length()-1);
			if(arr.length == 2)
				return Integer.parseInt(substring);
			
		} catch (IOException e) {
			e.printStackTrace();
		}
		return 60000;

	}
}
