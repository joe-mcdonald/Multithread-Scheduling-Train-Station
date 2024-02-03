# Multithread-Scheduling-Train-Station
C program that uses threads, mutexes and condition variables to handle trains entering and leaving a train station.

This multi threaded c program is designed to simulate trains in two directions 
crossing a single track. The data structure for the Trains includes attributes 
for the train�s ID, load time, cross time, priority, direction and state. Each 
train has its own thread, enabling concurrent execution and a condition variable 
called granted to signal when the train can cross the track. An array called trains[] 
holds all the train structures. The global variables last_direction, track_occupied 
and consecutive_same_direction track the last train to cross the tracks direction, 
track whether the track is occupied or not and to count the number of times a train 
crosses in one direction over and over again. The program uses a mutex called 
track_mutex to ensure that only one train can access the track at any given time. 
The condition variable track_available signals the waiting train threads when the 
track becomes available. The helper functions current_time_str() provides a formatted 
string of the current time for the programs output. The select_next_train() identifies 
what train is next to cross and train_behavior() manages the life cycle of each train 
thread, which includes sleeping for the loading and crossing time as well as changing 
its states. The output includes the current time as well as notifications for when a 
train becomes ready, starts crossing the main track and when it leaves the main track.
