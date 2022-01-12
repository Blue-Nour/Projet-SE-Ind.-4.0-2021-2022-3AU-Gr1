} 
void loop(){
    if (btnValue == 1 && task1_handle != NULL){
        vTaskSuspend(task2_handle);
        vTaskResume(task1_handle);}
    else if (btnValue ==4){
        vTaskSuspend(task1_handle);
        vTaskSuspend(task2_handle);}
    else {
        vTaskSuspend(task1_handle);
        vTaskResume(task2_handle);}
}
