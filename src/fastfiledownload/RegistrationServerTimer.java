package fastfiledownload;
import java.util.Timer;
import java.util.TimerTask;


/**
 * 
 * @author rkandur
 * 
 *         Class which is used to track the timer value of latest registration
 *         request from each directory server.
 *
 */
public class RegistrationServerTimer {
	
	// boolean which validates the directory server
	boolean m_valid = true;

	public RegistrationServerTimer() {
		Timer timer = new Timer();
		timer.schedule(new ChangeValueTask(), 5 * 1000);
	}

	class ChangeValueTask extends TimerTask {
		public void run() {
			m_valid = false;
		}
	} 

	public boolean isValid(){
		return m_valid;
	}
	
}

