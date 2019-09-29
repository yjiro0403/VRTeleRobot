#include <stdio.h>
#include <stdlib.h>	/* exit(), atoi()  */
#include <unistd.h>	/* sleep(), usleep() */
#include <signal.h>	/* sigcatch() */
#include <pthread.h>
/* IPC */
#include <ipc.h>
#include "../common/ipc_msg.h"
#include "../common/hoge.h"

/* Aria */
#include "Aria.h"

#define	MODULE_NAME	"ipc_test02"


/*** IPC ***/
bool		g_flag_listen;

pthread_t	g_ipc_listen_thread;

/* multi-threaded IPC communication */
pthread_mutex_t g_mutex_ipc;


hoge01		g_hoge01;
hoge02		g_hoge02;

static void	ipc_init(void);
static void	ipc_close(void);
static void	*ipc_listen(void *arg);
void		hoge01Handler(MSG_INSTANCE ref, void *data, void *dummy);
void		sigcatch(int sig);

int setVel = 0;
int setRotVel = 0;

class ConnHandler
{
public:
  // Constructor
  ConnHandler(ArRobot *robot);
  // Destructor, its just empty
  ~ConnHandler(void) {}
  // to be called if the connection was made
  void connected(void);
  // to call if the connection failed
  void connFail(void);
  // to be called if the connection was lost
  void disconnected(void);
protected:
  // robot pointer
  ArRobot *myRobot;
  // the functor callbacks
  ArFunctorC<ConnHandler> myConnectedCB;
  ArFunctorC<ConnHandler> myConnFailCB;
  ArFunctorC<ConnHandler> myDisconnectedCB;
};

/*
  The constructor, note its use of contructor chaining to initalize the 
  callbacks.
*/
ConnHandler::ConnHandler(ArRobot *robot) :
  myConnectedCB(this, &ConnHandler::connected),  
  myConnFailCB(this, &ConnHandler::connFail),
  myDisconnectedCB(this, &ConnHandler::disconnected)

{
  // set the robot poitner
  myRobot = robot;

  // add the callbacks to the robot
  myRobot->addConnectCB(&myConnectedCB, ArListPos::FIRST);
  myRobot->addFailedConnectCB(&myConnFailCB, ArListPos::FIRST);
  myRobot->addDisconnectNormallyCB(&myDisconnectedCB, ArListPos::FIRST);
  myRobot->addDisconnectOnErrorCB(&myDisconnectedCB, ArListPos::FIRST);
}

// just exit if the connection failed
void ConnHandler::connFail(void)
{
  printf("Failed to connect.\n");
  myRobot->stopRunning();
  Aria::shutdown();
  return;
}

// turn on motors, and off sonar, and off amigobot sounds, when connected
void ConnHandler::connected(void)
{
  printf("Connected\n");
  myRobot->comInt(ArCommands::SONAR, 0);
  myRobot->comInt(ArCommands::ENABLE, 1);
  myRobot->comInt(ArCommands::SOUNDTOG, 0);
}

// lost connection, so just exit
void ConnHandler::disconnected(void)
{
  printf("Lost connection\n");
  exit(0);
}


int main(int argc, char *argv[])
{
  std::string str;
  int ret;
  ArTime start;

    /* Set signal */
    if (SIG_ERR == signal(SIGINT, sigcatch)) {
	fprintf(stderr, "Fail to set signal handler\n");
	exit(1);
    }
    
    /* Initialize mutex for pthread */
    pthread_mutex_init(&g_mutex_ipc, NULL);
    
    /* Initialize IPC */
    ipc_init();

    /* Start IPC listening and publishing threads */
    g_flag_listen  = true;
    if (pthread_create(&g_ipc_listen_thread, NULL, &ipc_listen, NULL) != 0)
	perror("pthread_create()\n");
    
    //Aria init
     // connection to the robot
    ArSerialConnection con;
    // the robot
    ArRobot robot;
  // the connection handler from above
    ConnHandler ch(&robot);
    
    // init area with a dedicated signal handling thread
    Aria::init(Aria::SIGHANDLE_THREAD);
    
    // open the connection with the defaults, exit if failed
    if ((ret = con.open("/dev/ttyUSB0")) != 0)
  {
    str = con.getOpenMessage(ret);
    printf("Open failed: %s\n", str.c_str());
    Aria::shutdown();
    return 1;
  }

    // set the robots connection
    robot.setDeviceConnection(&con);
    // try to connect, if we fail, the connection handler should bail
    if (!robot.blockingConnect())
      {
	// this should have been taken care of by the connection handler
	// but just in case
	printf(
	       "asyncConnect failed because robot is not running in its own thread.\n");
	Aria::shutdown();
	return 1;
      }
    // run the robot in its own thread, so it gets and processes packets and such
    robot.runAsync(false);
    
    int prev_Rot=0;
    int k = 2; //velocity gain
    //Main roop
    while (g_flag_listen == true) {
      //motion commands
      robot.lock();
      robot.setVel(setVel);
      robot.setRotVel((setRotVel-prev_Rot)*k);
      //robot.setHeading(setRotVel);
      // robot.setDeltaHeading(setRotVel/4);
      prev_Rot = setRotVel;
      robot.unlock();
      start.setToNow();
      while (1)
      {
        robot.lock();
	//break if over 500m sec or chenge head state
	if (start.mSecSince() > 600 ||
	    setRotVel-prev_Rot > 80  ||
	    setRotVel-prev_Rot < -80)
	  {
	    robot.unlock();
	    break;
	  }   
	//printf("Trans: %10g Rot: %10g\n", robot.getVel(), robot.getRotVel());
	printf("setRotVel-prev_Rot = %d\n",setRotVel-prev_Rot);
	robot.unlock();
	ArUtil::sleep(100);
      }
      //sleep(1);
    }


    //Aria destract
    robot.disconnect();
    Aria::shutdown();

    /* wait thread join */
    g_flag_listen  = false;
    pthread_join(g_ipc_listen_thread, NULL);
    
    /* Close IPC */
    ipc_close();
    
    pthread_mutex_destroy(&g_mutex_ipc);
    
    return 0;
}


static void ipc_init(void)
{
    /* Connect to the central server */
    if (IPC_connect(MODULE_NAME) != IPC_OK) {
	fprintf(stderr, "IPC_connect: ERROR!!\n");
	exit(-1);
    }
    
    IPC_defineMsg(HOGE01_MSG, IPC_VARIABLE_LENGTH, HOGE01_MSG_FMT);
    IPC_subscribeData(HOGE01_MSG, hoge01Handler, NULL);
    
    IPC_defineMsg(HOGE02_MSG, IPC_VARIABLE_LENGTH, HOGE02_MSG_FMT);
}


static void ipc_close(void)
{
    /* close IPC */
    fprintf(stderr, "Close IPC connection\n");
    IPC_disconnect();
}


static void *ipc_listen(void *arg)
{
    static long i = 0;
    
    fprintf(stderr, "Start ipc_listen\n");
    while (g_flag_listen == true) {
      //change time
	if (i % 20 == 0 ) fprintf(stderr, "IPC_listen: (%ld)\n", i);
	
	pthread_mutex_lock(&g_mutex_ipc);
	IPC_listen(10);	// 10[msec]
	pthread_mutex_unlock(&g_mutex_ipc);
	
	i++;
    }
    fprintf(stderr, "Stop ipc_listen\n");
}

void sigcatch(int sig)
{
    fprintf(stderr, "Catch signal %d\n", sig);

    /* wait thread join */
    pthread_join(g_ipc_listen_thread, NULL);
    
    /* Close IPC */
    ipc_close();
    
    pthread_mutex_destroy(&g_mutex_ipc);
    
    exit(1);
}


void hoge01Handler(MSG_INSTANCE ref, void *data, void *dummy)
{
    int i, num;
    
    g_hoge01 = *(hoge01 *)data;
    setVel = g_hoge01.setVel;
    setRotVel = g_hoge01.setRotVel;
    //printf("g_hoge01.setRotVel = %d\n", g_hoge01.setRotVel); 
}
