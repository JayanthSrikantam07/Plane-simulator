#include <iostream>
#include <cstdlib>
#include <queue>
#include <algorithm>
#include <thread>
#include <semaphore.h>
#include <chrono>  // for high_resolution_clock

using namespace std;

// Plane Class to keep name, service Type and time
class Plane {
    private :
        std::string name;
        std::string serviceType;
        int sequenceId;
        char type;
    public :
        Plane(std::string name, std::string serviceType, int id) : name(name), serviceType(serviceType), sequenceId(id), type(toupper(serviceType.at(0))) {}
        
        // cout operator overloading by declaring it friend of Plane Class 
        friend std::ostream& operator<<(std::ostream& out, const Plane& o);
};

// << cout operator overloading to print the rectangle object
std::ostream& operator<<(std::ostream& out, const Plane& o)  
{

    out << o.name  << " "  << o.type << o.sequenceId  << " " <<  o.serviceType ;
    return out;
}


// run the simulation and loop in all thread until run == 1
bool run = true;

//departurePlan Counter
int departureCounter = 1;

//arrivalPlane Counter
int arrivalCounter = 1;

//departure Service Counter
int departureServiceCounter = 0;

//arrival Service Counter
int arrivalServiceCounter = 0;



// declaring the shared resource runway
bool runwayAvailable = true;

// declaring arrival, departure, serviced Queue
queue<Plane> arrivalQueue;
queue<Plane> departureQueue;

//declaring the semaphore run for stopping the thread
sem_t runSimulation;

//declaring the semaphore run for  syncronization arrival queue
sem_t arrivalSem; 

//declaring the semaphore run for  syncronization departure queue 
sem_t departureSem;

//declaring the semaphore for runway syncronization 
sem_t runwaySem;


void displayMenu() {
    cout << "Choose an option:" << endl;
    cout << "(1) Print Statistics" << endl;
    cout << "(2) Stop Simulation" << endl;
}

void planeCreation() {

  // setting the loop condition before servicing another departure 
  sem_wait(&runSimulation);
  bool loop = run;
  sem_post(&runSimulation);
  
  while(loop) {
      
      // sleep for 0 to 5 seconds
      int n =  rand() % 6;
      this_thread::sleep_for(chrono::seconds(n));
      
      //setting name of the plane
      string name = "Flight#";
      
      string type = "";
      if ( rand() % 2 == 0 ) { 
        type = "departed";
      }
      else {
        type = "arrived";
      }

      Plane *p = nullptr;
      
      // creating a plane object according to type of plane and add it to corrsponding queue
      // by taking semaphore lock on corrsponding semaphore
      if (type.compare("departed") == 0 ) {
        p = new Plane(name,type,departureCounter);
        
        // incrementing departureCounter for assiging id
        departureCounter++;

        // taking a lock on departureSem semaphore to update queue
        sem_wait(&departureSem);
        
        //adding new departure plane into  departure Queue 
        departureQueue.push(*p);

        // releasing lock by sem_post on departureSem
        sem_post(&departureSem);
        
      }
      else {
        p = new Plane(name,type,arrivalCounter);

        // incrementing arrivalCounter  for assiging id
        arrivalCounter++;

        // taking a lock on arrivalSem semaphore to update queue
        sem_wait(&arrivalSem);
        
        //adding new arrival plane into arrival Queue 
        arrivalQueue.push(*p);
        
        // releasing lock by sem_post on arrivalSem 
        sem_post(&arrivalSem);

      }
      
      // Checking again the loop condition before creating another plane
      sem_wait(&runSimulation);
      loop = run;
      sem_post(&runSimulation);
  }
}

void arrivalService() {

  // setting the loop condition before servicing another departure 
  sem_wait(&runSimulation);
  bool loop = run;
  sem_post(&runSimulation);
 
  // readyArrival variable is used for to check runway is available
  bool readyArrival;
  
  while(loop) {
      
      // this_thread::sleep_for for 10 to 20 seconds
      int n =  10 + rand() % 11;
      this_thread::sleep_for(chrono::seconds(n));
      
      readyArrival = false;
      
      // Checks if runway variable accessibility
      sem_wait(&runwaySem);
      if (runwayAvailable == true ) {
          readyArrival = runwayAvailable;
          runwayAvailable = false;
      }
      sem_post(&runwaySem);
    
      if ( readyArrival == true ) {
        
        // taking arrivalSem lock for arrivalQueue
        sem_wait(&arrivalSem);
        
        if (!arrivalQueue.empty()) {
          
          Plane p = arrivalQueue.front();
          
          // updating the arrival plane service counter 
          arrivalServiceCounter++;

          // removing element from arrivalQueue
          arrivalQueue.pop();
        
          // priting the plane which is removed
          cout << p << endl;

        }
        
        // releasing the queue lock
        sem_post(&arrivalSem);
        
        // setting the runway variable availability to true
        sem_wait(&runwaySem);
        runwayAvailable = true;
        sem_post(&runwaySem);
      } 
      
      // Checking again the loop condition before servicing another plane arrival 
      sem_wait(&runSimulation);
      loop = run;
      sem_post(&runSimulation);
  }
}

void departureService() {

  // setting the loop condition before servicing another departure 
  sem_wait(&runSimulation);
  bool loop = run;
  sem_post(&runSimulation);
  bool readyDeparture;
  
  while(loop) {

      // this_thread::sleep_for for 10 to 25 seconds
      int n =  10 + rand() % 16;
      this_thread::sleep_for(chrono::seconds(n));
      
      readyDeparture = false;
      
      // Checks if runway variable accessibility
      sem_wait(&runwaySem);
      if (runwayAvailable == true ) {
          readyDeparture = runwayAvailable;
          runwayAvailable = false;
      }
      sem_post(&runwaySem);
    
      if ( readyDeparture == true ) {
        
        // taking departureSem lock for departureQueue 
        sem_wait(&departureSem);
        if (!departureQueue.empty()) {
          Plane p = departureQueue.front();
        
          // removing the front from departure queue
          departureQueue.pop();
        
          // updating the departureServiceCounter 
          departureServiceCounter++;
          
          // printing the plane which is just departed
          cout << p << endl;
          
        }
        
        // releasing the queue lock on departureQueue
        sem_post(&departureSem);

        
        // setting the runway variable availability to true
        sem_wait(&runwaySem);
        runwayAvailable = true;
        sem_post(&runwaySem);
      } 
      
      // Checking again the loop condition before servicing another departure 
      sem_wait(&runSimulation);
      loop = run;
      sem_post(&runSimulation);
  }
}

// Getting user choice
int getInput() {
  int choice;
  while (true) {
    cin >> choice;
    if (choice < 1 || choice > 2) {
      cout << "Please enter either 1 or 2 " << endl;
    }
    else{
      return choice;
    }
  }
}

int main(int argc, char *argv[]) {

    cout << "********************************************************" << endl;
    cout << "            Welcome to Airport Simulator                " << endl;
    cout << "********************************************************" << endl;
    
    int choice;
    
    // setting seed with current time counter
    srand(time(nullptr));

    // Recording start time 
    auto start = std::chrono::high_resolution_clock::now();

    // Initializing the Semaphore
    sem_init(&runSimulation, 0, 0);
    sem_init(&runwaySem, 1, 1);
    sem_init(&arrivalSem, 1, 1);
    sem_init(&departureSem, 1, 1);
    
    // plane creation pctid thread creation
    thread pctid(planeCreation);

    // arrival service astid thread creation
    thread astid(arrivalService);
    
    // departure service  dstid thread creation 
    thread dstid(departureService);
      
    // Start the simulation by incrementing the runSimulation to 1
    sem_post(&runSimulation);    
    
    while (run) {
      displayMenu();
      choice = getInput();
      switch(choice) {
        case 1: cout << "Planes Serviced: " << endl;
                cout << "-----------------" << endl;
                cout << "Departure Serviced: " << departureServiceCounter << endl;
                cout << "Arrival Serviced: " << arrivalServiceCounter << endl;
                cout << "Waiting Queue: "<< endl;
                cout << "-----------------" << endl;
                cout << "Departure Queue: " << departureQueue.size() << endl;
                cout << "Arrival Queue: " << arrivalQueue.size() << endl;
                break;
        case 2: 
                // setting run to false for terminating the simulation
                sem_wait(&runSimulation);
                run = false;
                sem_post(&runSimulation);
                break;
        default : cout << "Wrong Choice." << endl;
      }
    }
    
    // syncronizeing all thread
    pctid.join();
    astid.join();
    dstid.join();
    
    // Record end time
    auto finish = std::chrono::high_resolution_clock::now();
    
    cout << "Process returned 0 (0x0) " ;
    std::chrono::duration<double> duration = finish - start ;
    cout << "execution time : " << duration.count() << " seconds" << endl; 
    

    // Destroying the shared variable semaphore
    sem_destroy(&runSimulation);
    sem_destroy(&runwaySem);
    sem_destroy(&arrivalSem);
    sem_destroy(&departureSem);
    return 0;
}
