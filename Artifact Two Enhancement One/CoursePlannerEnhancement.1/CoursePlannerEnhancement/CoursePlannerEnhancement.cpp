/*
* Name: CoursePlanner.cpp
* Author: Nick Silvestro
* Description: Program to load, print, search for course information
* Date: 8/13/23
*/


#include <iostream>
#include <fstream>
#include <string> 
#include <vector>

#include "CSVparser.hpp"

using namespace std;

/*
* Global definitions
*/

// Defines a structure to hold course information
struct Course {
	string courseNumber; // unique identifier
	string courseName;
	string prerequ1;
	string prerequ2;
	Course() {
		prerequ1 = "";
		prerequ2 = "";
	}
};


// Hash Table class definition
class HashTable {

private:
	// Defines node structures to hold courses
	struct Node {
		Course course;
		unsigned int key;
		Node* next;

		// default constructor
		Node() {
			key = UINT_MAX;
			next = nullptr;
		}

		// initialize with a course
		Node(Course aCourse) : Node() {
			course = aCourse;
		}

		// initialize with a course and a key
		Node(Course aCourse, unsigned int aKey) : Node(aCourse) {
			key = aKey;
		}
	};

	vector<Node> nodes;
	unsigned int tableSize = 547; // Large prime used for hashing

	unsigned int hash(int key);


// Public methods
public:
	HashTable();
	void InsertCourse(Course course);
	void PrintAll();
	Course SearchForCourse(string courseNumber);
	void DisplayCourseOutlook(string courseNumber);
};


// To track loaded files
vector<string> loadedFiles{};


// Default constructor
HashTable::HashTable() {
	// Initialize node structure by resizing tableSize
	nodes.resize(tableSize);
}


// Calculate hash values
unsigned int HashTable::hash(int key) {
	// return key tableSize
	return key % tableSize;
}


// Insert a course into the hash table
void HashTable::InsertCourse(Course course) {
	// create the key for the given course using string, loop to allow for alphanumeric combination
	string stringToProcess = "000";
	int num = 0;
	for (int i = 4; i < course.courseNumber.length(); i++) {
		stringToProcess.at(num) = course.courseNumber.at(i);
		num++;
	}

	// create the key for the course
	unsigned key = hash(atoi(stringToProcess.c_str()));

	// retrieve node using key
	Node* oldNode = &(nodes.at(key));

	// if no entry found for the key
	if (oldNode == nullptr) {
		// assign this node to the key position
		Node* newNode = new Node(course, key);
		nodes.insert(nodes.begin() + key, (*newNode));
	}

	// else if node is not used
	else {
		// assigning old node key to UNIT_MAX, set to key, set old node to course and old node next to null pointer
		if (oldNode->key == UINT_MAX) {
			oldNode->key = key;
			oldNode->course = course;
			oldNode->next = nullptr;
		}
		else {
			// else find the next open node
			while (oldNode->next != nullptr) {
				oldNode = oldNode->next;
			}
			// add new newNode to end
			oldNode->next = new Node(course, key);
		}
	}
}


// Print all courses
void HashTable::PrintAll() {
	// iterate through the nodes
	for (int i = 0; i < nodes.size(); ++i) {
		Node* node = &(nodes.at(i));

		//   if key not equal to UINT_MAx
		if (node->key != UINT_MAX) {
			// output course number and course name
			std::cout << node->course.courseNumber << ", " << node->course.courseName << endl;
			// node is equal to next iter
			node = node->next;

			// while node not equal to nullptr
			while (node != nullptr) {

				// output course number and course name
				std::cout << node->course.courseNumber << ", " << node->course.courseName << endl;
				// node is equal to next node
				node = node->next;
			}
		}
	}
}


//Search for a given course
Course HashTable::SearchForCourse(string courseNumber) {
	Course course;

	// create the key for the given course
	string stringToProcess = "000";
	int num = 0;
	for (int i = 4; i < courseNumber.length(); i++) {
		stringToProcess.at(num) = courseNumber.at(i);
		num++;
	}

	unsigned key = hash(atoi(stringToProcess.c_str()));

	Node* node = &(nodes.at(key));

	// if entry found for the key
	if (node != nullptr && node->key != UINT_MAX && node->course.courseNumber.compare(courseNumber) == 0) {
		//return node course
		return node->course;
	}

	// if no entry found for the key
	if (node == nullptr || node->key == UINT_MAX) {
		// return course
		return course;
	}

	// while node not equal to nullptr
	while (node != nullptr) {
		// if the current node matches, return it
		if (node->key != UINT_MAX && node->course.courseNumber.compare(courseNumber) == 0) {
			return node->course;
		}
		//node is equal to next node
		node = node->next;
	}
	return course;
}


// Display the course information to the user
void DisplayCourse(Course course) {
	cout << course.courseNumber << ", " << course.courseName << endl;
	// Handle variable number of prerequisites
	if (course.prerequ1 != "") {
		cout << "Prerequisites: " << course.prerequ1;
	}
	if (course.prerequ2 != "") {
		cout << ", " << course.prerequ2;
	}
	cout << endl;
	return;
}


// Display a course's outlook to user (required as a prerequ)
void HashTable::DisplayCourseOutlook(string courseNumber) {
	// Get current course
	Course currentCourse = SearchForCourse(courseNumber);
	cout << currentCourse.courseName << "'s Outlook:" << endl;
	cout << endl;

	cout << "Courses requiring this course as a prerequisite:" << endl;

	// Loop through all courses
	for (int i = 0; i < nodes.size(); ++i) {
		Node* node = &(nodes.at(i));

		//   if key not equal to UINT_MAx
		if (node->key != UINT_MAX) {
			// output course number and course name if current course is a prerequ
			if (node->course.prerequ1 != "") {
				if ((node->course.prerequ1) == (currentCourse.courseNumber)) {
					std::cout << node->course.courseNumber << ", " << node->course.courseName << endl;
				}
			}
			if (node->course.prerequ2 != "") {
				if ((node->course.prerequ2) == (currentCourse.courseNumber)) {
					std::cout << node->course.courseNumber << ", " << node->course.courseName << endl;
				}
			}
			
			// node is equal to next iter
			node = node->next;

			// while node not equal to nullptr
			while (node != nullptr) {
				// output course number and course name if current course is a prerequ
				if (node->course.prerequ1 != "") {
					if ((node->course.prerequ1) == (currentCourse.courseNumber)) {
						std::cout << node->course.courseNumber << ", " << node->course.courseName << endl;
					}
				}
				if (node->course.prerequ2 != "") {
					if ((node->course.prerequ1) == (currentCourse.courseNumber)) {
						std::cout << node->course.courseNumber << ", " << node->course.courseName << endl;
					}
				}
				node = node->next;
			}
		}
	}
}


// Input validation on file path entered
bool FilePathChecker(string csvPath) {
	string extension = ".csv";
	bool badPath = false;
	bool existingOpenedFile = false;

	// Check for presence of csv file extension
	while ((csvPath.length() < extension.length()) || (csvPath.substr((csvPath.size() - 4), 4) != extension)) {
		cout << "File name must end in .csv extention" << endl;
		cout << endl;

		return badPath;
	}

	// Ensure file can be opened 
	ifstream csvFile;
	csvFile.open(csvPath);
	
	if (csvFile) {
		// Opened
		cout << "Existing file found." << endl;
		cout << endl;

		csvFile.close();
		loadedFiles.push_back(csvPath); // Add to vector of file names

		existingOpenedFile = true;
		return existingOpenedFile;
	}
	else {
		// Not opened
		cout << "File could not be opened or no existing file was found." << endl;
		cout << endl;

		return existingOpenedFile;
	}
}


// Function to ensure duplicate files are not loaded
bool DuplicateFileChecker(string csvPath) {
	for (int i = 0; i < loadedFiles.size(); i++) {
		// if file name in list of file names
		if (loadedFiles.at(i) == csvPath) {
			cout << "File has already been loaded." << endl;
			return true;
			break;
		}
	}
	return false;
}


// Function to load a CSV file of course to hash table
void LoadCourses(string csvPath, HashTable* hashTable) {
	cout << "Attempting to parse CSV file..." << endl;

	// initialize the CSV Parser using the given path
	csv::Parser file = csv::Parser(csvPath);

	int count = 0;

	try {
		// loop to read rows of a CSV file
		for (unsigned int i = 0; i < file.rowCount(); i++) {

			// Create a data structure and add to the collection of courses
			Course course;
			course.courseNumber = file[i][0];
			course.courseName = file[i][1];
			// Handles varying number of prerequisites
			if (file[i].size() > 2) {
				course.prerequ1 = file[i][2];
			}
			if (file[i].size() > 3) {
				course.prerequ2 = file[i][3];
			}

			// push this course to the end
			hashTable->InsertCourse(course);
			++count;
		}
	}
	// error handling
	catch (csv::Error& e) {
		std::cerr << e.what() << std::endl;
	}
	// validation
	cout << "Parsing complete: " << count << " courses loaded" << endl;
}


// Function to validate user coursekey input
bool CourseKeyFormatCheck(string courseKey) {
	string errorMessage = "Course key format is four letters followed by three numbers (ex. CSCI100)";

	// Length validation
	if (courseKey.length() != 7) {
		cout << errorMessage << endl;
		cout << endl;
		return false;
	}
	// Alphanumeric pattern validation
	else {
		for (int i = 0; i < 4; i++) {
			if (!isalpha(courseKey.at(i))) {
				cout << errorMessage << endl;
				cout << endl;
				return false;
			}
		}

		for (int i = 4; i < 7; i++) {
			if (!isdigit(courseKey.at(i))) {
				cout << errorMessage << endl;
				cout << endl;
				return false;
			}
		}
		return true;
	}
}


// Function to display a menu of options to the user
void DisplayMenu() {
	cout << endl;
	cout << "===============================" << endl;
	cout << "Menu:" << endl;
	cout << "   1. Load Course Data (CSV Files)" << endl;
	cout << "   2. Print Course List" << endl;
	cout << "   3. Print a Course" << endl;
	cout << "   4. Display a Course Outlook" << endl;
	cout << "   9. Exit" << endl;
	cout << "===============================" << endl;

	cout << endl;
	cout << "Enter your choice: ";
}


// Main function
int main() {

	// Define course path, key variables
	string csvPath = "Courses.csv";
	string courseKey;

	// Flag variables
	bool fileOpened = false;
	bool duplicate = false;
	bool validCourseKey = false;

	// Creates a hash table to hold the courses
	HashTable* courseTable;
	Course course;
	courseTable = new HashTable();

	cout << "Welcome to the Course Planner:" << endl;

	int choice = 0;
	while (choice != 9) {
		DisplayMenu();
		cin >> choice;
		cout << endl;

		// Handle choices
		switch (choice) {
		case 1:
			while (!fileOpened) {
				cout << "What file would you like to load: ";
				cin >> csvPath;

				// Ensure file is not already loaded
				duplicate = DuplicateFileChecker(csvPath);

				// Check path if file is not a duplicate
				if (!duplicate) {
					fileOpened = FilePathChecker(csvPath);
				}
				else {
					break;
				}
				
			}

			// Load data if file is not a duplicate
			if (!duplicate) {
				LoadCourses(csvPath, courseTable);
			}
			
			// Reset flags
			duplicate = false;
			fileOpened = false;
			break;


		case 2:
			// Method call to print all courses
			courseTable->PrintAll();
			break;


		case 3:
			// Course key input validation loop
			while (!validCourseKey) {
				cout << "Enter a course number for more information: ";
				cin >> courseKey;

				validCourseKey = CourseKeyFormatCheck(courseKey);
			}

			// Ensure input is accepted if upper or lower case
			for (int i = 0; i <= 3; ++i) {
				if (islower(courseKey[i])) {
					courseKey[i] = toupper(courseKey[i]);
				}
			}

			// Method call to search for the course
			course = courseTable->SearchForCourse(courseKey);

			if (!course.courseNumber.empty()) {
				DisplayCourse(course);
			}
			else {
				cout << "Course number " << courseKey << " not found." << endl;
			}

			// Reset flags
			validCourseKey = false;
			break;


		case 4:
			// Course key input validation lop
			while (!validCourseKey) {
				cout << "Enter the number of the course you wish to see the outlook of: ";
				cin >> courseKey;

				validCourseKey = CourseKeyFormatCheck(courseKey);
			}

			// Ensure input is accepted if upper or lower case
			for (int i = 0; i <= 3; ++i) {
				if (islower(courseKey[i])) {
					courseKey[i] = toupper(courseKey[i]);
				}
			}

			courseTable->DisplayCourseOutlook(courseKey);
			
			// Reset flags
			validCourseKey = false;
			break;


		case 9:
			cout << "Exiting now." << endl;
			break;


		default:
			// Input validation for menu options
			cout << choice << " is not a valid menu option." << endl;
			cout << endl;
			break;
		}
	}

	// Memory deallocation
	delete courseTable;

	cout << "Goodbye!" << endl;

	return 0;
}
