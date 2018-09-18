# FIE
A basic Fault Injection Environment for FreeRTOS developed and tested on STM32F3DISCOVERY board.

### Aim
This Fault Injection Environment has been developed in order to test the robustness of some parts of FreeRTOS and eventually to give a suggestion to improve (harden) it. Four features of the kernel have been identified as good injection points:
  * Global FreeRTOS variables
  * TCB (Task Control Block)
  * Task lists
  * Queues, semaphores and mutexes
The whole project is composed by two parts: *FIEbrd* which is the board-side set of files and *FIEhst* which is the host-side set of scripts. The idea is the following one: the board under test and the host computer setup a connection, then the computer sends the injection parameters to the board and finally it waits for the injection to be done by the board; when this event occurs, the injection results are sent back to the computer and saved in a file for further analysis.
