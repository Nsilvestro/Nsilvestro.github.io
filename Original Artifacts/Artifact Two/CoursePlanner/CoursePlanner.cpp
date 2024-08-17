/*
* Name: CoursePlanner.cpp
* Author: Nick Silvestro
* Description: Program to load, print, search for course information
* Date: 8/13/23
*/


#include <iostream>
#include <string> 
#include <vector>

#include "CSVparser.hpp"

using namespace std;

// Global definitions
// 
// define a structure to hold course information
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
	// Define structures to hold courses
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
	unsigned int tableSize = 547; // Large prime for hashing

	unsigned int hash(int key);

public:
	HashTable();
	void Insert(Course course);
	void PrintAll();
	Course Search(string courseNumber);
};


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
void HashTable::Insert(Course course) {
	// create the key for the given course- using string, loop to allow for alphanumeric combination
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
Course HashTable::Search(string courseNumber) {
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
void displayCourse(Course course) {
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


// Load CSV file of course to hash table
void loadCourses(string csvPath, HashTable* hashTable) {
	cout << "Loading CSV file " << csvPath << endl;

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
			hashTable->Insert(course);
			++count;
		}
	}
	catch (csv::Error& e) {
		std::cerr << e.what() << std::endl;
	}
	cout << count << " courses loaded" << endl;
}


// Main function
int main() {

	// Define course path, key variables
	string csvPath = "Courses.csv";
	string courseKey;

	cout << "Welcome to the Course Planner." << endl;
	cout << endl;

	// Create a hash table to hold the courses
	HashTable* courseTable;

	Course course;
	courseTable = new HashTable();

	int choice = 0;

	while (choice != 9) {
		cout << "============================" << endl;
		cout << "Menu:" << endl;
		cout << "   1. Load Data Structure" << endl;
		cout << "   2. Print Course List" << endl;
		cout << "   3. Print a Course" << endl;
		cout << "   9. Exit" << endl;
		cout << "============================" << endl;

		cout << endl;
		cout << "Enter your choice: ";
		cin >> choice;

		switch (choice) {
		case 1:
			// Get user input for a file name
			cout << "What file would you like to load?" << endl;
			cin >> csvPath;

			// Method call to load courses
			loadCourses(csvPath, courseTable);
			break;

		case 2:
			// Method call to print all courses
			courseTable->PrintAll();
			break;

		case 3:
			cout << "Enter a course number for more information: ";
			// Get user input for course to search
			cin >> courseKey;

			// Ensure input is accepted if upper or lower case
			for (int i = 0; i <= 3; ++i) {
				if (islower(courseKey[i])) {
					courseKey[i] = toupper(courseKey[i]);
				}
			}

			// Method call to search for the course
			course = courseTable->Search(courseKey);

			if (!course.courseNumber.empty()) {
				displayCourse(course);
			}
			else {
				cout << "Course number " << courseKey << " not found." << endl;
			}
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

	cout << "Goodbye!" << endl;

	return 0;
}
