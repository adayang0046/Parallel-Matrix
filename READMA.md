Run order:
- have your tests.csv ready, Data format: P,M,N,Q,Method (int,int,int,int,(SERIAL/1D/2D))
- chmod +x RUNAll.sh 
- bash RUNAll.sh or ./RUNAll.sh         -> check if your results.csv looks good
- pip install pandas if you haven't 
- python average.py                     -> check if your averaged_results.csv looks good (P,M,N,Q,Method,Average_Time_sec)
- python final.py                       -> check if your final_results.csv looks good (P,M,N,Q,Method,Average_Time_sec,Average_Time_sec_Serial,Speedup,Cost)