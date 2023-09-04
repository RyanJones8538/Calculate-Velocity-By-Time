// VelocityCalculator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

struct PositionCoordinates
{
	double latitudeDegrees = 0;
	double latitudeRadians = 0;
	double longitude = 0;
	double height = 0;
};

class WGS84ECEFVelocityCalculator
{
	const double a = 6378137.000;
	double aSquared = pow(a, 2);
	const double b = 6356752.31424518;
	double bSquared = pow(b, 2);

	static const int maxNumEntries = 1000;

	double radius;

	double timeIncrement;

	int totalEntriesFound = 0;

	static const int valsPerEntry = 4;

	double positionData[maxNumEntries][valsPerEntry];

	/// <summary>
	/// Calculate total distance travelled
	/// </summary>
	double calculateDistanceBetweenPoints(double x1, double x2, double y1, double y2, double z1, double z2)
	{
		return sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2) + pow((z2 - z1), 2));
	}

	double calculateECEFPositionsAndDistance(int lowIndex)
	{
		int highIndex = lowIndex + 1;

		PositionCoordinates position1, position2;
		position1.latitudeDegrees = positionData[lowIndex][1];
		position1.latitudeRadians = convertDegreesToRadians(positionData[lowIndex][1]);
		position1.longitude = convertDegreesToRadians(positionData[lowIndex][2]);
		position1.height = positionData[lowIndex][3];

		position2.latitudeDegrees = positionData[highIndex][1];
		position2.latitudeRadians = convertDegreesToRadians(positionData[highIndex][1]);
		position2.longitude = convertDegreesToRadians(positionData[highIndex][2]);
		position2.height = positionData[highIndex][3];

		double radius1 = calculateRadius(position1.latitudeDegrees);
		double radius2 = calculateRadius(position2.latitudeDegrees);
		double x1, x2, y1, y2, z1, z2;

		x1 = convertLLAToECEFX(position1.latitudeRadians, position1.longitude, position1.height, radius1);
		x2 = convertLLAToECEFX(position2.latitudeRadians, position2.longitude, position2.height, radius2);
		y1 = convertLLAToECEFY(position1.latitudeRadians, position1.longitude, position1.height, radius1);
		y2 = convertLLAToECEFY(position2.latitudeRadians, position2.longitude, position2.height, radius2);
		z1 = convertLLAToECEFZ(position1.latitudeRadians, position1.height, radius1);
		z2 = convertLLAToECEFZ(position2.latitudeRadians, position2.height, radius2);

		return calculateDistanceBetweenPoints(x1, x2, y1, y2, z1, z2);
	}

	/// <summary>
	/// Find Radius of Curvature
	/// </summary>
	double calculateRadius(double latitude)
	{
		double eSquared = (aSquared - bSquared) / aSquared;
		double sineSquared = .5 * (1 - cos(convertDegreesToRadians(2 * latitude)));
		double denominator = sqrt(1 - (eSquared * sineSquared));
		return a / denominator;
	}

	/// <summary>
	/// Divide Distance by Time
	/// </summary>
	double calculateVelocity(double distance, double time)
	{
		return distance / time;
	}

	/// <summary>
	/// Convert LLA to ECEF X-coordinate
	/// </summary>
	double convertLLAToECEFX(double latitude, double longitude, double altitude, double radius)
	{
		//Radius + altitude could be calculated once, and inputted as parameter to both this X and the Y function.
		//But that would decrease readability for negligible performance-gain.
		return (radius + altitude) * (cos(latitude) * cos(longitude));
	}

	/// <summary>
	/// Convert LLA to ECEF Y-coordinate
	/// </summary>
	double convertLLAToECEFY(double latitude, double longitude, double altitude, double radius)
	{
		return (radius + altitude) * (cos(latitude) * sin(longitude));
	}

	/// <summary>
	/// Convert LLA to ECEF Z-coordinate
	/// </summary>
	double convertLLAToECEFZ(double latitude, double altitude, double radius)
	{
		return ((bSquared / aSquared) * radius + altitude) * sin(latitude);
	}

	/// <summary>
	/// Function to call velocity for given time and print results
	/// </summary>
	void callVelocity(double timeVal)
	{
		if (isTimeValid(timeVal) == false)
		{
			return;
		}

		string choice;

		int lowIndex = searchIndicesBinary(timeVal, 0, totalEntriesFound - 1);

		if (lowIndex < 0 || lowIndex >= totalEntriesFound)
		{
			cout << "ERROR" << endl;
			return;
		}

		//prevents seg-fault and gives means to estimate final time
		if (lowIndex == totalEntriesFound - 1)
		{
			lowIndex--;
		}

		double distance = calculateECEFPositionsAndDistance(lowIndex);

		cout << "Velocity at " << to_string(timeVal) <<
			" seconds from UNIX epoch: " << calculateVelocity(distance, timeIncrement)
			<< " meters per second." << endl;
		cout << "Enter any key to continue." << endl;
		cin >> choice;
		return;
	}

	/// <summary>
	/// Find Radius of Curvature
	/// </summary>
	double convertDegreesToRadians(double x)
	{
		return (3.14159265358979323846 * x) / 180;
	}

	/// <summary>
	/// Fill Position Array with dummy-values
	/// </summary>
	void initializeArray()
	{
		for (int i = 0; i < maxNumEntries; i++)
		{
			for (int j = 0; j < valsPerEntry; j++)
			{
				positionData[i][j] = 0;
			}
		}
	}

	/// <summary>
	/// Check whether time is measured by position chart
	/// </summary>
	bool isTimeValid(double timeVal)
	{
		string choice;
		if (positionData[0][0] > timeVal)
		{
			cout << "Value too small for time-range." << endl;
			cout << "Minimum Value: " << to_string(positionData[0][0]) << endl;
			cout << "Enter any key to continue." << endl;
			cin >> choice;
			return false;
		}
		if (positionData[totalEntriesFound - 1][0] < timeVal)
		{
			cout << "Value too large for time-range." << endl;
			cout << "Maximum Value: " << to_string(positionData[totalEntriesFound - 1][0]) << endl;
			cout << "Enter any key to continue." << endl;
			cin >> choice;
			return false;
		}
		//Assignment states to regard initial position's velocity as 0 m/s
		if (positionData[0][0] == timeVal)
		{
			cout << "Starting Position: Velocity Calculated as 0." << endl;
			cout << "Enter any key to continue." << endl;
			cin >> choice;
			return false;
		}
		return true;
	}
	
	/// <summary>
	/// Read .csv and fill info into positionData array
	/// </summary>
	void parseCSV()
	{
		ifstream file("SciTec_code_problem_data.csv");
		char delimiter = ',';
		string line, val;
		int col = 0;
		for (int row = 0; row < maxNumEntries; row++)
		{
			col = 0;
			getline(file, line);

			if (!file.good())
				break;
			totalEntriesFound++;
			stringstream iss(line);
			
			while (getline(iss, val, delimiter))
			{
				if (col >= valsPerEntry)
				{
					break;
				}
				if (col == 3)
				{
					positionData[row][col] = stod(val) * 1000;
				}
				else 
				{
					positionData[row][col] = stod(val);
				}
				col++;
			}
		}
		//I am assuming the time increments are constant between all entries
		timeIncrement = positionData[1][0] - positionData[0][0];
	}
	
	/// <summary>
	/// Find correct index by time
	/// </summary>
	int searchIndicesBinary(double timeVal, int low, int high)	
	{
		if (low > high)
		{
			return -1;
		}
		else
		{
			int mid = (low + high) / 2;
			double timeGap = timeVal - positionData[mid][0];

			if (timeGap >= 0 && timeGap < timeIncrement)
			{
				return mid;
			}

			else if (positionData[mid][0] < timeVal)
			{
				return searchIndicesBinary(timeVal, mid + 1, high);
			}
			else
			{
				return searchIndicesBinary(timeVal, low, mid - 1);
			}
		}
	}


public:
	/// <summary>
	/// Serves as entry-point into the class
	/// </summary>
	void mainMenu()
	{
		bool keepRunning = true;
		while (keepRunning)
		{
			cout << "Please Select An Option." << endl;
			cout << "1. Evaluate Velocity at 1532334000 Seconds From UNIX Epoch: " << endl;
			cout << "2. Evaluate Velocity at 1532335268 Seconds From UNIX Epoch: " << endl;
			cout << "3. Evaluate Velocity at Custom Time From UNIX Epoch: " << endl;
			cout << "4. Quit" << endl;
			char choice;
			string timeVal;
			cin >> choice;

			switch (choice)
			{
			case '1':
				callVelocity(1532334000);
				break;
			case '2':
				callVelocity(1532335268);
				break;
			case '3':
				cout << "Enter Your Time." << endl;
				cin >> timeVal;
				callVelocity(stod(timeVal));
				break;
			case '4':
				keepRunning = false;
				cout << "Have A Lovely Day." << endl;
				break;
			default:
				cout << "Not a Valid Input" << endl;
				cout << "-----------------------" << endl;
				cout << "Enter Any Input To Continue" << endl;
				cin >> choice;
				break;
			}
		}
		return;
	}
	WGS84ECEFVelocityCalculator() {
		initializeArray();
		parseCSV();
	}

	~WGS84ECEFVelocityCalculator() {

	}

};

WGS84ECEFVelocityCalculator *WGS84ECEFVelocityCalculatorFactory()
{
	return new WGS84ECEFVelocityCalculator;
}


/// <summary>
/// Entry-point of Program
/// </summary>
int main()
{
	WGS84ECEFVelocityCalculator* ConsoleLogger = WGS84ECEFVelocityCalculatorFactory();
	ConsoleLogger->mainMenu();
	delete ConsoleLogger;
	return 0;
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
