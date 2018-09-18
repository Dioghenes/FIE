# FIE
A basic Fault Injection Environment for FreeRTOS.

## Aim
This Fault Injection Environment has been developed in order to test the robustness of some parts of FreeRTOS and eventually to give a suggestion to improve (harden) it. Four features of the kernel have been identified as good injection points:
  * Global FreeRTOS variables
  * TCB (Task Control Block)
  * Task lists
  * Queues, semaphores and mutexes

