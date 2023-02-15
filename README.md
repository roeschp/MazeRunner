# MazeRunner
Simple application to solve mazes in .txt file.

## Trémaux' algorithm
This solving method marks each corridor when it is passed. For every second run, this corridor is marked a second time and may not be entered again afterwards.

## Rules 
For this application there are some specifc rules for the selection of direction when entering a crossroad.
The ranking order is:

1. Downwards
2. Right
3. Upwards
4. Left

It is not necessary in this order but is have to be defined, for the application to decide the ranking.

## Playground definitions

⬜ &ensp; White fields are just simple corridors. 

⬛ &ensp; Black fields are walls and can not be entered.

🟨 &ensp; Yellow marker are corridors this the player has passed once so far

🟥 &ensp; Red marker are corrisors this the player pas passed two times so far and can not be entered again afterwards. 

🟦 &ensp; The blue marker is the destination of the maze

❎ &ensp; Green fields are the shortest way from start to destination.


## Advantages of the Trémaux' methodology 
With this solving algorithm it is possible, to solve complex maze with even a circle network. But one big problem for this method, it is not always possible to found the shortest way back to the start position even.

## Fieldscanning 
To read in the current maze textfile, be sure that the first line contains the matrix of the following maze.
The process to read the playground is from top to bottom and left to right. When found a crossroad the marker is already set for the upcoming line, so the algorithm will skip this when read in the next line.

````
Scan from top to bottom and left to right
            -->
            |	1 1 0 1 1
            v   1 1 M 1 1
                0 M 0 M 0
Skip the M:	    > > M >

````

It checks all direction for existing crossroads and mark them as usual.

````
If there are more than two corridors mark everyone
				
				 Example 1       Example 2
				   | |			   | |
				___|M|___	       |M|___
				__M   M__		   |  M__
				   |M|			   |M|
				   | |			   | |
````