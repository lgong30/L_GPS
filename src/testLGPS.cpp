#include <iostream>
#include "L_GPS_Tester.hpp"

int main()
{
	std::string input("C:/Users/gtuser/Desktop/demo_packet_schedulers/packets.dat");

	try{
		L_GPS_Tester lgps(input);
		lgps.print();
		lgps.run();	
	}
	catch(std::runtime_error& e)
	{
		std::cout << "Encounter runtime error while running the simulator: \n"
		          << "  " << e.what() << std::endl;
	}


	return 0;

}