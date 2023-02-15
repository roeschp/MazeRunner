#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <direct.h>
#include <stdbool.h>

// Basic settings for the algorithm
#define TARGET_FILE "spielfeldtest.txt"
#define SPEED_STANDARD 50
#define SHOW_SETTINGS_TIME 2000

// Helper to get maze field
#define TRAILING_ZERO 1
#define BUFFER_SIZE 20

// Limits for the maze size
#define MAZE_SIZE_MIN 5
#define MAZE_SIZE_MAX 50

// ASCII characters to show in console
#define ROBOTER "R"
#define EMPTY_SPACE 32
#define BLOCK 219

// Limit for the maximum steps on marker
#define LIMIT_STEPS_ON_MARKER 3

// fonts color
#define FBLACK      "\033[30;"
#define FCYAN       "\x1b[36m"
#define D_FGREEN    "\033[6;"

// background color
#define BBLACK      "40m"
#define BRED        "41m"
#define BGREEN      "42m"
#define BYELLOW     "43m"
#define BBLUE       "44m"
#define BWHITE      "47m"
#define D_BGREEN    "46m"

// Default console color
#define DEFAULT_COLOR	"\033[0m"

// Types to differentiate the content
typedef enum mazeType
{
	Corridor,
	Wall,
	Destination,
	Marker
}mazeType;

// Base structure for all elements from the maze content
typedef struct
{
	mazeType type;
	bool markOne;
	bool markTwo;
}element;

// Helper function
char* getFieldByCurrentWorkingDirectory(char fileName[]);
COORD getDimension(char string[]);
char** scanFieldFromPath(char* pathToField, COORD* dimension);
void printMaze2Console(element** markField, COORD dimension);
bool validateInput(char** field, COORD dimension, COORD startPosition);
void printObject2Console(HANDLE hConsole, COORD coord, char object[], char colorFont[], char colorBack[]);
element** getMazeContent(COORD dimension, char** field);

// Maze solving algorithm
void startMazeSolver(char* path, COORD startPosition, int speed);
void tremaux(HANDLE hConsole, element** fieldContent, COORD dimension, COORD startPosition, int speed);
COORD firstRule(element** markField, COORD currentCoord, COORD latestCoord);
COORD secondRule(element** markField, COORD currentCoord);

// Get shortes way back 
COORD getNextStepBack(element** markField, COORD currentCoord, COORD latestCoord);
int getWayBack(HANDLE hConsole, element** markField, COORD startPosition, COORD mazeDesination, int speed);

int main(int argc, char* argv[])
{
	// Enter your settings
	char* path2TargetFile = getFieldByCurrentWorkingDirectory(TARGET_FILE);
	int speed = SPEED_STANDARD;
	COORD startPosition = { 0 };
	startPosition.X = 1;
	startPosition.Y = 1;

	printf("Use field from:\n%s\n", path2TargetFile);
	printf("\nStart position X:%d Y:%d \n", startPosition.X, startPosition.Y);
	printf("\nSpeed is set to %dms\n", speed);
	Sleep(SHOW_SETTINGS_TIME);

	// Start solving the maze
	startMazeSolver(path2TargetFile, startPosition, speed);

	exit(0);
}

/// <summary>
/// Master function for solving maze with the Trémaux'-algorithm 
/// </summary>
/// <param name="path">where the maze.txt is located</param>
/// <param name="startPosition">coordination X and Y where to start</param>
/// <param name="speed">how fast the steps is clocked in milliseconds</param>
void startMazeSolver(char* path, COORD startPosition, int speed)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD dimension = { 0 };

	// Get field from selected path and calculate dimension
	char** field = scanFieldFromPath(path, &dimension);

	if (field != NULL)
	{
		// Check that the field is valid
		if (validateInput(field, dimension, startPosition) != TRUE)
		{
			printObject2Console(hConsole, dimension, "Error - the maze with the settings are not valid!\n", D_FGREEN, BBLACK);
			exit(1);
		}

		// Create a mirrow field with all marker elements
		element** mazeContent = getMazeContent(dimension, field);

		// Print the field to console
		printMaze2Console(mazeContent, dimension);

		if (mazeContent == NULL)
		{
			printObject2Console(hConsole, dimension, "Error! - Something went wrong when creating the content of this field!\n", D_FGREEN, BBLACK);
			exit(1);
		}

		// Start the algorithm to solve the maze
		tremaux(hConsole, mazeContent, dimension, startPosition, speed);

		// Free the memory for the field
		for (int index = 0; index < dimension.Y; index++)
		{
			free(mazeContent[index]);
		}

		free(mazeContent);
		free(path);
	}
	else
	{
		printObject2Console(hConsole, dimension, "Error - Something went wrong when scanning field\n", D_FGREEN, BBLACK);
		exit(1);
	}
}

/// <summary>
/// Create path with the target filename in the current execution folder
/// </summary>
/// <param name="fileName">as target with the maze content</param>
/// <returns>Complete string with the path</returns>
char* getFieldByCurrentWorkingDirectory(char fileName[])
{
	char buffer[FILENAME_MAX];

	// Get current working directory of the execution
	if (_getcwd(buffer, FILENAME_MAX) == NULL)
	{
		printf("Error - can not find current working directory\n");
		exit(1);
	}

	// Get buffer length
	size_t lengthBuffer = strlen(buffer);

	buffer[lengthBuffer] = '\\';

	// Append the target file name to the path
	for (int i = 1; i <= sizeof(TARGET_FILE); i++)
	{
		buffer[lengthBuffer + i] = fileName[i - 1];
	}

	// Calculate the path length and create a dynamically memory for the name
	size_t pathLength = lengthBuffer + sizeof(TARGET_FILE) + TRAILING_ZERO;
	char* exePath = (char*)calloc(pathLength, sizeof(char));

	if (exePath == NULL)
	{
		printf("Error - Failed to reserve dynamic memory\n");
		exit(1);
	}

	// Copy the path to the dynamically character array
	errno_t errNo = strcpy_s(exePath, pathLength, buffer);

	if (errNo != 0)
	{
		printf("Error - Failed to create execution Path\n");
		exit(1);
	}

	return exePath;
}

/// <summary>
/// Trim the dimension of the maze from the string
/// </summary>
/// <param name="string">that is to be split</param>
/// <returns>Dimension of the maze in coordination</returns>
COORD getDimension(char string[])
{
	COORD coord = { 0 };

	if (string == NULL)
		return coord;

	char* delimiter = " ";
	char* ptr;
	char* nextToken = NULL;

	// Split the X coordination from the string
	ptr = strtok_s(string, delimiter, &nextToken);

	if (ptr != NULL)
		coord.X = atoi(ptr);

	// Split the Y coordination from the string
	ptr = strtok_s(NULL, delimiter, &nextToken);

	if (ptr != NULL)
		coord.Y = atoi(ptr);

	return coord;
}

/// <summary>
/// Iterate through the target file.txt and copy all 0 and 1 into a dynamically array
/// </summary>
/// <param name="pathToField">for the target .txt</param>
/// <param name="dimension">of the maze in coordination</param>
/// <returns>Dynamically array with the content of the maze</returns>
char** scanFieldFromPath(char* pathToField, COORD* dimension)
{
	if (pathToField == NULL)
		return NULL;

	FILE* file;
	errno_t errNo = fopen_s(&file, pathToField, "r");

	// When can not opened the file then exit
	if (errNo != 0)
	{
		printf("Error! - this file can not be open\n");
		exit(1);
	}
	else
	{
		// Read first line of .txt to get dimension
		char buffer[BUFFER_SIZE];
		fgets(buffer, BUFFER_SIZE, file);

		*dimension = getDimension(buffer);

		// Create a dynamically array with the dimensions
		char** field = (char**)calloc(dimension->Y, sizeof(char*));

		for (int index = 0; index < dimension->Y; index++)
		{
			field[index] = calloc(dimension->X, sizeof(char));
		}

		if (field == NULL)
		{
			printf("Error - Failed to reserve memory\n");
			exit(1);
		}

		int indexX = 0;
		int indexY = 0;
		char tempBuffer = 0;

		// Iterate through file until end of file
		while ((tempBuffer = fgetc(file)) != EOF)
		{
			if (tempBuffer == '\n')
			{
				indexX = 0;
				indexY += 1;
			}
			else if (tempBuffer != EMPTY_SPACE)
			{
				field[indexY][indexX] = tempBuffer;
				indexX += 1;
			}

		}

		fclose(file);
		return field;
	}
}

/// <summary>
/// Display the maze to the console, with all diffrent types
/// </summary>
/// <param name="markArray">with the content of the maze</param>
/// <param name="dimension">the size of the maze</param>
void printMaze2Console(element** elementField, COORD dimension)
{
	// Function system to use cmd for terminal
	// cls to clear complete terminal
	system("cls");

	// Iterate through the content of the maze
	for (int indexY = 0; indexY < dimension.Y; indexY++)
	{
		for (int indexX = 0; indexX < dimension.X; indexX++)
		{
			if (elementField[indexY][indexX].type == Wall)
				printf(FBLACK BBLACK"%c"DEFAULT_COLOR, BLOCK);

			if (elementField[indexY][indexX].type == Corridor)
				printf(FBLACK BWHITE" "DEFAULT_COLOR);

			if (elementField[indexY][indexX].type == Marker)
				printf(FBLACK BWHITE" "DEFAULT_COLOR);

			if (elementField[indexY][indexX].type == Destination)
				printf(FBLACK BBLUE" "DEFAULT_COLOR);
		}

		printf("\n");
	}
}

/// <summary>
/// Validate maze size is between 5x5 and 50x50 and
/// check start position is not a maze wall or the destination
/// </summary>
/// <param name="field">- from textdocument</param>
/// <param name="dimension">- X and Y size of field</param>
/// <param name="startPosition">- X and Y of start position</param>
/// <returns>True when all conditions passed</returns>
bool validateInput(char** field, COORD dimension, COORD startPosition)
{
	if (field == NULL)
		return FALSE;

	// Validate that the size is in the middle of the limits 
	if (dimension.Y < MAZE_SIZE_MIN || dimension.X < MAZE_SIZE_MIN)
	{
		printf("Error - the dimension [ %d | %d ] is to short!\n", dimension.X, dimension.Y);
		printf("Please try again with a larger maze.\n");
		exit(1);
	}
	if (dimension.Y > MAZE_SIZE_MAX || dimension.X > MAZE_SIZE_MAX)
	{
		printf("Fcked the dimension [ %d | %d ] is to large!\n", dimension.X, dimension.Y);
		printf("Please try again with a smaller maze.\n");
		exit(1);
	}

	// Validate that the start position is not inside a wall
	if (field[startPosition.Y][startPosition.X] == '1')
	{
		printf("Error - start position can not be inside a wall!\n");
		printf("Please try again, with another start position than X:%d Y:%d.\n", startPosition.X, startPosition.Y);
		exit(1);
	}
	// Validate that the start position is not the maze destination
	if (field[startPosition.Y][startPosition.X] == 'X')
	{
		printf("Error - start position can not be the maze destination!\n");
		printf("Please try again, with another start position than X:%d Y:%d.\n", startPosition.X, startPosition.Y);
		exit(1);
	}

	return TRUE;
}

/// <summary>
/// Console display to print located characters with color
/// </summary>
/// <param name="hConsole">target at which to print</param>
/// <param name="coord">location in console</param>
/// <param name="object">character to print</param>
/// <param name="colorFont">font to dye</param>
/// <param name="colorBack">background to dye</param>
void printObject2Console(HANDLE hConsole, COORD coord, char object[], char colorFont[], char colorBack[])
{
	SetConsoleCursorPosition(hConsole, coord);

	printf("%s", colorFont);
	printf("%s", colorBack);
	printf("%s", object);
	printf(DEFAULT_COLOR);
}

/// <summary>
/// Scan maze from scratch textfile. Mark every start of a corridor when there are more than two branches.
/// </summary>
/// <param name="dimension">of the maze in the form of coordinations</param>
/// <param name="field">the scratch maze with 0 and 1</param>
/// <returns></returns>
element** getMazeContent(COORD dimension, char** field)
{
	// Catch that the field is correct
	if (field == NULL)
	{
		return NULL;
	}

	// Create dynamically memory for the maze with the dimension
	element** markArray = (element**)calloc(dimension.Y, sizeof(element*));

	for (int index = 0; index < dimension.Y; index++)
	{
		markArray[index] = calloc(dimension.X, sizeof(element));
	}

	if (markArray == NULL)
	{
		printf("Error! - can not reserve memory for the %dx%d\n", dimension.X, dimension.Y);
		exit(1);
	}

	for (int indexY = 0; indexY < dimension.Y; indexY++)
	{
		for (int indexX = 0; indexX < dimension.X; indexX++)
		{
			// Create the wall type
			if (field[indexY][indexX] == '1')
			{
				element content;
				content.type = Wall;
				markArray[indexY][indexX] = content;
				continue;
			}

			// Create the destination type
			if (field[indexY][indexX] == 'X')
			{
				element content;
				content.type = Destination;
				markArray[indexY][indexX] = content;
				continue;
			}

			/*
			* Scan from top to bottom and left to right
					   -->
					  |	1 1 0 1 1
					  v 1 1 M 1 1
						0 M 0 M 0
			Skip the M:	> > M >

			*/

			// When array is already marked skip this one
			if (markArray[indexY][indexX].type == Marker)
			{
				continue;
			}

			// Check all directions if there is another corridor 
			if (indexX > 0 && indexY > 0 && (field[indexY][indexX] == '0' || field[indexY][indexX] == 'X'))
			{
				// Count up all corridors possibilitys
				int countCorners = 0;

				// Check above for a corridor
				if (field[indexY - 1][indexX] == '0')
					countCorners++;

				// Check right for a corridor
				if (field[indexY][indexX + 1] == '0')
					countCorners++;

				// Check below for a corridor
				if (field[indexY + 1][indexX] == '0')
					countCorners++;

				// Check left for a corridor
				if (field[indexY][indexX - 1] == '0')
					countCorners++;

				// If there are more than two corridors mark everyone
				/*
				* Example 1   Example 2
				   | |			   | |
				___|M|___	       |M|___
				__M   M__		   |  M__
				   |M|			   |M|
				   | |			   | |
				*/

				if (countCorners > 2)
				{
					if (countCorners > 3)
					{
						// More than 3 corridors -> everything have to be marked
						if (field[indexY - 1][indexX] != 'X')
							markArray[indexY - 1][indexX].type = Marker;

						if (field[indexY][indexX + 1] != 'X')
							markArray[indexY][indexX + 1].type = Marker;

						if (field[indexY + 1][indexX] != 'X')
							markArray[indexY + 1][indexX].type = Marker;

						if (field[indexY][indexX - 1] != 'X')
							markArray[indexY][indexX - 1].type = Marker;

						// Check center is not marked
						if (markArray[indexY][indexX].type != Marker)
							markArray[indexY][indexX].type = Corridor;

						continue;
					}
					else
					{
						// Check which direction is a corridor and mark this one
						if (field[indexY - 1][indexX] == '0')
							markArray[indexY - 1][indexX].type = Marker;

						if (field[indexY][indexX + 1] == '0' && field[indexY][indexX + 1] != 'X')
							markArray[indexY][indexX + 1].type = Marker;

						if (field[indexY + 1][indexX] == '0' && field[indexY + 1][indexX] != 'X')
							markArray[indexY + 1][indexX].type = Marker;

						if (field[indexY][indexX - 1] == '0' && field[indexY][indexX - 1] != 'X')
							markArray[indexY][indexX - 1].type = Marker;

						// Check center is not marked
						if (markArray[indexY][indexX].type != Marker)
							markArray[indexY][indexX].type = Corridor;

						continue;
					}
				}
				else
				{
					// When there are less then three corridors it is a normal corridor
					if (markArray[indexY][indexX].type != Marker)
					{
						markArray[indexY][indexX].type = Corridor;
					}
				}
			}
		}
	}

	return markArray;
}

/// <summary>
/// Algorithm to solve complexe maze with calculation to get the way back to source
/// </summary>
/// <param name="hConsole">where to display the maze solving</param>
/// <param name="elementField">content of the maze with all types</param>
/// <param name="dimension">of the maze in coordination</param>
/// <param name="startPosition">the source position</param>
/// <param name="speed">Delay time in milliseconds</param>
void tremaux(HANDLE hConsole, element** elementField, COORD dimension, COORD startPosition, int speed)
{
	if (elementField == NULL)
	{
		printObject2Console(hConsole, dimension, "Error - Something went wrong when start the algorithm!\n", D_FGREEN, BBLACK);
		exit(1);
	}

	COORD latestCoord = { 0 };
	COORD currentCoord = startPosition;
	COORD nextCoord = startPosition;

	char emptySpace[] = " ";

	// Iterate the algorithm until the current coordination is the destination
	while (elementField[nextCoord.Y][nextCoord.X].type != Destination)
	{
		// Set current Coordination
		currentCoord = nextCoord;

		// get next Coordination by passing by the rules
		nextCoord = firstRule(elementField, currentCoord, latestCoord);

		if (nextCoord.X == 0 && nextCoord.Y == 0)
		{
			printObject2Console(hConsole, dimension, "Error - Maze has no solution!\n", D_FGREEN, BBLACK);
			exit(1);
		}

		// set latest Coordination
		latestCoord = currentCoord;

		printObject2Console(hConsole, nextCoord, ROBOTER, FBLACK, BWHITE);

		// Clear the latest position of the Roboter
		if (elementField[currentCoord.Y][currentCoord.X].type != Marker)
			printObject2Console(hConsole, latestCoord, emptySpace, FBLACK, BWHITE);

		// When step on a marker tag this one
		if (elementField[currentCoord.Y][currentCoord.X].type == Marker)
		{
			// If this marker has not been used yet, tag this for the first time
			if (elementField[currentCoord.Y][currentCoord.X].markOne == FALSE)
			{
				elementField[currentCoord.Y][currentCoord.X].markOne = TRUE;
				SetConsoleCursorPosition(hConsole, currentCoord);
				printf(FBLACK BYELLOW" "DEFAULT_COLOR);
			}
			// If this marker is already taged once, mark this a second time
			else if (elementField[currentCoord.Y][currentCoord.X].markTwo == FALSE)
			{
				elementField[currentCoord.Y][currentCoord.X].markTwo = TRUE;
				SetConsoleCursorPosition(hConsole, currentCoord);
				printf(FBLACK BRED" "DEFAULT_COLOR);
			}
		}


		Sleep(speed);
	}

	// Display the solution
	if (elementField[nextCoord.Y][nextCoord.X].type == Destination)
	{
		printObject2Console(hConsole, dimension, "Found one way to the destination\n", D_FGREEN, BBLACK);
	}

	Sleep(SHOW_SETTINGS_TIME);

	// Start the algorithm to find the way back to the source
	int stepsBack = getWayBack(hConsole, elementField, startPosition, nextCoord, speed);

	SetConsoleCursorPosition(hConsole, dimension);
	printf(D_FGREEN BBLACK"Shortes way to destination in %d steps\n"DEFAULT_COLOR, stepsBack);
	Sleep(SHOW_SETTINGS_TIME);
}

/// <summary>
/// You do not talk about Fight Club!
/// Calculate the next coordination for the maze and compare all types from the field
/// </summary>
/// <param name="markField">Maze field with all types and tag values</param>
/// <param name="currentCoord">Coordination to calculate all directions</param>
/// <param name="latestCoord">Coordination to compare with possible next step</param>
/// <returns>Coordination when all conditions for the first Trémaux' rule are passed</returns>
COORD firstRule(element** markField, COORD currentCoord, COORD latestCoord)
{
	// Check all direction in the following sequence 
	/*
			Third
			  ^
			  |
	Fourth < - - > Second
			  |
			  v
			First
	*/

	// Check bottom direction is not the latest position and not a wall
	if ((markField[currentCoord.Y + 1][currentCoord.X].type != Wall)
		&& (currentCoord.Y + 1 != latestCoord.Y || currentCoord.X != latestCoord.X))
	{
		// Check bottem direction is not a tag otherwise call second rule
		if (markField[currentCoord.Y + 1][currentCoord.X].type != Marker)
		{
			currentCoord.Y++;
			return currentCoord;
		}
		else
		{
			// The next position is a tag so check all other tags value
			return secondRule(markField, currentCoord);
		}
	}

	// Check right direction is not the latest position and not a wall
	else if ((markField[currentCoord.Y][currentCoord.X + 1].type != Wall)
		&& (currentCoord.Y != latestCoord.Y || currentCoord.X + 1 != latestCoord.X))
	{
		// Check right direction is not a tag otherwise call second rule
		if (markField[currentCoord.Y][currentCoord.X + 1].type != Marker)
		{
			currentCoord.X++;
			return currentCoord;
		}
		else
		{
			// The next position is a tag so check all other tags value
			return secondRule(markField, currentCoord);
		}
	}

	// Check top direction is not the latest position and not a wall
	else if ((markField[currentCoord.Y - 1][currentCoord.X].type != Wall)
		&& (currentCoord.Y - 1 != latestCoord.Y || currentCoord.X != latestCoord.X))
	{
		// Check top direction is not a tag otherwise call second rule
		if (markField[currentCoord.Y - 1][currentCoord.X].type != Marker)
		{
			currentCoord.Y--;
			return currentCoord;
		}
		else
		{
			// The next position is a tag so check all other tags value
			return secondRule(markField, currentCoord);
		}
	}

	// Check left direction is not the latest position and not a wall
	else if ((markField[currentCoord.Y][currentCoord.X - 1].type != Wall)
		&& (currentCoord.Y != latestCoord.Y || currentCoord.X - 1 != latestCoord.X))
	{
		// Check left direction is not a tag otherwise call second rule
		if (markField[currentCoord.Y][currentCoord.X - 1].type != Marker)
		{
			currentCoord.X--;
			return currentCoord;
		}
		else
		{
			// The next position is a tag so check all other tags value
			return secondRule(markField, currentCoord);
		}
	}

	return currentCoord;
}

/// <summary>
/// You do !NOT! talk about Fight Club!
/// When step on a tag element check all others and go to this direction with the lowest tag value
/// </summary>
/// <param name="markField">Maze field with all types and tag values</param>
/// <param name="currentCoord">Coordination to get present position</param>
/// <returns>Coordination when all conditions for the second Trémaux' rule are passed - returns 0:0 if maze is not solvable</returns>
COORD secondRule(element** markField, COORD currentCoord)
{
	int botDirection = 0;
	int rightDirection = 0;
	int topDirection = 0;
	int leftDirection = 0;

	// Check if bottom is a marker and count the value
	if (markField[currentCoord.Y + 1][currentCoord.X].type == Marker)
	{
		botDirection++;

		if (markField[currentCoord.Y + 1][currentCoord.X].markOne == TRUE)
			botDirection++;

		if (markField[currentCoord.Y + 1][currentCoord.X].markTwo == TRUE)
			botDirection++;

		// When bottom is a marker with value 0 return bottom 
		if (botDirection == 1)
		{
			currentCoord.Y++;
			return currentCoord;
		}
	}
	// Check if right is a marker and count the value
	if (markField[currentCoord.Y][currentCoord.X + 1].type == Marker)
	{
		rightDirection++;

		if (markField[currentCoord.Y][currentCoord.X + 1].markOne == TRUE)
			rightDirection++;

		if (markField[currentCoord.Y][currentCoord.X + 1].markTwo == TRUE)
			rightDirection++;

		// When right is a marker with value 0 return right 
		if (rightDirection == 1)
		{
			currentCoord.X++;
			return currentCoord;
		}
	}
	// Check if top is a markerand count the value
	if (markField[currentCoord.Y - 1][currentCoord.X].type == Marker)
	{
		topDirection++;

		if (markField[currentCoord.Y - 1][currentCoord.X].markOne == TRUE)
			topDirection++;

		if (markField[currentCoord.Y - 1][currentCoord.X].markTwo == TRUE)
			topDirection++;

		// When top is a marker with value 0 return top 
		if (topDirection == 1)
		{
			currentCoord.Y--;
			return currentCoord;
		}
	}
	// Check if left is a marker and count the value
	if (markField[currentCoord.Y][currentCoord.X - 1].type == Marker)
	{
		leftDirection++;

		if (markField[currentCoord.Y][currentCoord.X - 1].markOne == TRUE)
			leftDirection++;

		if (markField[currentCoord.Y][currentCoord.X - 1].markTwo == TRUE)
			leftDirection++;

		// When left is a marker with value 0 return left 
		if (leftDirection == 1)
		{
			currentCoord.X--;
			return currentCoord;
		}
	}

	// Compare all values with bottom tag value
	if (botDirection > 0 && botDirection < LIMIT_STEPS_ON_MARKER)
	{
		if (rightDirection <= 0 && topDirection <= 0 && leftDirection <= 0)
		{
			currentCoord.Y++;
			return currentCoord;
		}
		else if (botDirection <= rightDirection)
		{
			currentCoord.Y++;
			return currentCoord;
		}
		else if (botDirection <= topDirection)
		{
			currentCoord.Y++;
			return currentCoord;
		}
		else if (botDirection <= leftDirection)
		{
			currentCoord.Y++;
			return currentCoord;
		}
	}
	// Compare all values with right tag value
	if (rightDirection > 0 && rightDirection < LIMIT_STEPS_ON_MARKER)
	{
		if (topDirection <= 0 && leftDirection <= 0)
		{
			currentCoord.X++;
			return currentCoord;
		}
		else if (rightDirection <= topDirection)
		{
			currentCoord.X++;
			return currentCoord;
		}
		else if (rightDirection <= leftDirection)
		{
			currentCoord.X++;
			return currentCoord;
		}
	}
	// Compare all values with top tag value
	if (topDirection > 0 && topDirection < LIMIT_STEPS_ON_MARKER)
	{
		if (leftDirection <= 0)
		{
			currentCoord.Y--;
			return currentCoord;
		}
		else if (topDirection <= leftDirection)
		{
			currentCoord.Y--;
			return currentCoord;
		}
	}
	// Compare all values with left tag value
	if (leftDirection > 0 && leftDirection < LIMIT_STEPS_ON_MARKER)
	{
		currentCoord.X--;
		return currentCoord;
	}

	// When stepping into this the maze is not solvable
	currentCoord.X = 0;
	currentCoord.Y = 0;

	return currentCoord;
}

/// <summary>
/// Helper to get the shortes way back to source from destination with the tagged maze content
/// </summary>
/// <param name="markField">maze with the markings and their values</param>
/// <param name="currentCoord">Coordination to get present position</param>
/// <param name="latestCoord">Coordination to compare with possible next step</param>
/// <returns>Coordination with the next step to get back to source - returns latest coordination if something went wrong</returns>
COORD getNextStepBack(element** markField, COORD currentCoord, COORD latestCoord)
{
	// Check bottom is a marker with only one tag and not latest step
	if ((currentCoord.Y + 1 != latestCoord.Y || currentCoord.X != latestCoord.X)
		&& (markField[currentCoord.Y + 1][currentCoord.X].type == Marker)
		&& (markField[currentCoord.Y + 1][currentCoord.X].markOne == TRUE)
		&& (markField[currentCoord.Y + 1][currentCoord.X].markTwo == FALSE))
	{
		currentCoord.Y++;
		return currentCoord;
	}

	// Check right is a marker with only one tag and not latest step
	else if ((currentCoord.Y != latestCoord.Y || currentCoord.X + 1 != latestCoord.X)
		&& (markField[currentCoord.Y][currentCoord.X + 1].type == Marker)
		&& (markField[currentCoord.Y][currentCoord.X + 1].markOne == TRUE)
		&& (markField[currentCoord.Y][currentCoord.X + 1].markTwo == FALSE))
	{
		currentCoord.X++;
		return currentCoord;
	}

	// Check top is a marker with only one tag and not latest step
	else if ((currentCoord.Y - 1 != latestCoord.Y || currentCoord.X != latestCoord.X)
		&& (markField[currentCoord.Y - 1][currentCoord.X].type == Marker)
		&& (markField[currentCoord.Y - 1][currentCoord.X].markOne == TRUE)
		&& (markField[currentCoord.Y - 1][currentCoord.X].markTwo == FALSE))
	{
		currentCoord.Y--;
		return currentCoord;
	}

	// Check left is a marker with only one tag and not latest step
	else if ((currentCoord.Y != latestCoord.Y || currentCoord.X - 1 != latestCoord.X)
		&& (markField[currentCoord.Y][currentCoord.X - 1].type == Marker)
		&& (markField[currentCoord.Y][currentCoord.X - 1].markOne == TRUE)
		&& (markField[currentCoord.Y][currentCoord.X - 1].markTwo == FALSE))
	{
		currentCoord.X--;
		return currentCoord;
	}

	// If there is no marker with one tag, check for the next corridor which is not the latest step
	if ((currentCoord.Y + 1 != latestCoord.Y || currentCoord.X != latestCoord.X)
		&& (markField[currentCoord.Y + 1][currentCoord.X].type == Corridor))
	{
		currentCoord.Y++;
		return currentCoord;
	}
	else if ((currentCoord.Y != latestCoord.Y || currentCoord.X + 1 != latestCoord.X)
		&& (markField[currentCoord.Y][currentCoord.X + 1].type == Corridor))
	{
		currentCoord.X++;
		return currentCoord;
	}
	else if ((currentCoord.Y - 1 != latestCoord.Y || currentCoord.X != latestCoord.X)
		&& (markField[currentCoord.Y - 1][currentCoord.X].type == Corridor))
	{
		currentCoord.Y--;
		return currentCoord;
	}
	else if ((currentCoord.Y != latestCoord.Y || currentCoord.X - 1 != latestCoord.X)
		&& (markField[currentCoord.Y][currentCoord.X - 1].type == Corridor))
	{
		currentCoord.X--;
		return currentCoord;
	}
	// Return latest coordination if there is no possible direction 
	else
	{
		// Something went wrong
		return latestCoord;
	}
}

/// <summary>
/// Calculate the shortest way from destination to source in the maze. Thourgh the simply highlighted tags.
/// </summary>
/// <param name="hConsole">Console to print </param>
/// <param name="markField">maze with the markings and their values</param>
/// <param name="startPosition">Source must be adjusted back to</param>
/// <param name="mazeDesination">Destination of the maze, where to start from</param>
/// <param name="speed">Delay time in milliseconds</param>
/// <returns>Steps number from destination to source coordination</returns>
int getWayBack(HANDLE hConsole, element** markField, COORD startPosition, COORD mazeDesination, int speed)
{
	COORD nextCoord = mazeDesination;
	COORD latestCoord = { 0 };
	COORD currentCoord = { 0 };
	int countBack = 0;

	// Analyze the coordination until the current coordination is the source
	while ((nextCoord.X != startPosition.X) || (nextCoord.Y != startPosition.Y))
	{
		currentCoord = nextCoord;
		nextCoord = getNextStepBack(markField, currentCoord, latestCoord);

		// When get back the latest coordination something went wrong
		if (nextCoord.X == latestCoord.X && nextCoord.Y == latestCoord.Y)
		{
			printf("Error! - something went wrong when calculating the way back to source!\n");
			exit(1);
		}

		printObject2Console(hConsole, nextCoord, "X", FBLACK, BGREEN);

		latestCoord = currentCoord;
		countBack++;

		Sleep(speed);
	}

	return countBack;
}