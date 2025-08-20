#include <DueTimer.h>

/*PARAMETRI PROGETTUALI*/
#define N 2000                  //Ntot di campioni
#define Tc_mus 500              //Tc passo di campionamento -> fc = 5000Hz

#define TRANSDUCER_GAIN_V 100.0
#define TRANSDUCER_GAIN_C 2.0
#define GAIN_V (TRANSDUCER_GAIN_V*0.9987/412.654)	  //attenuazione tensione
#define GAIN_C (TRANSDUCER_GAIN_C*0.9985/274.487)	  //attenuazione corrente
#define OFFSET_V 2053.818		                        //lv 0 in digit
#define OFFSET_C 2054.551		                        //lv 0 in digit

const float ist=5;


/*VARIABILI*/
float tensione[N], corrente[N];		//buffer
int i=0;							//indice di caricamento

int READY_FLAG=0;   //legenda: 0-non processare, 1-processa prima metà, 2-processa seconda metà
boolean OVERRUN_FLAG=false;

int campioniMisura=-1;
float rmsV=0;
float rmsC=0;
float S=0;      //pot apparente
float P=0;      //pot attiva
float Q=0;      //pot reattiva



/*PROTOTPI FUNZIONI*/
void acquisisci();
int triggerFirst(float arr[], float isteresi, int size);
int triggerLast(float arr[], float isteresi, int size);
float potenzaAttiva(float tens[], float corr[], int numCampioni);
float RMS(float v[N], int numCampioni);
float componenteContinua(float v[N], int numCampioni);
void stampaVettore(float arr[], int dim);



void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Timer3.setPeriod(Tc_mus);
  Timer3.attachInterrupt(acquisisci).start();
}


void loop() {

	/*if(OVERRUN_FLAG)
		Serial.print("ERROREEEEEEEEEEEEEEEEEEEEEEEE!");
	*/
  
    if(READY_FLAG != 0)
    {

      if (READY_FLAG==2) //devo misurare la seconda parte
      {
        Serial.print("SECONDA META\n\n\n");

        /*
          Serial.print("Continua corrente: ");
          Serial.println(componenteContinua(corrente+N/2, N/2));

          Serial.print("Continua tensione: ");
          Serial.println(componenteContinua(tensione+N/2, N/2));
          */

        campioniMisura = triggerLast(tensione+N/2, ist, N/2) - triggerFirst(tensione+N/2, ist, N/2);

        if(campioniMisura <= 0) //caso peggiore, livelli di trigger non trovati
          Serial.println("Segnale fuori portata!!!");
        else
        {
            
          rmsC = RMS(corrente+N/2, campioniMisura);
          Serial.print("RMS corrente: ");
          Serial.print(rmsC);
		      Serial.println(" A");
		  

          rmsV = RMS(tensione+N/2, campioniMisura);
          Serial.print("RMS tensione: ");
          Serial.print(rmsV);
		      Serial.println(" V");

          P = potenzaAttiva(tensione+N/2, corrente+N/2, campioniMisura);
          Serial.print("Potenza attiva: ");
          Serial.print(P);
		      Serial.println(" W");

          S=rmsV * rmsC;
          Serial.print("Potenza apparente: ");
          Serial.print(S);
		      Serial.println(" VA");

          Q = sqrt(constrain((S*S) - (P*P), 0, 100000000));
          Serial.print("Potenza reattiva: ");
          Serial.print(Q);
		      Serial.println(" var");

          Serial.print("Fattore potenza: ");
          Serial.print(P/S);
		      Serial.println(" W/VA");
        }
      }
      else //if(READY_FLAG==1) devo misurare la prima parte
      {
        Serial.print("PRIMA META\n\n\n");

        /*
          Serial.print("Continua corrente: ");
          Serial.println(componenteContinua(corrente, N/2));

          Serial.print("Continua tensione: ");
          Serial.println(componenteContinua(tensione, N/2));
          */

        campioniMisura =  triggerLast(tensione, ist, N/2) - triggerFirst(tensione, ist, N/2);

        if(campioniMisura <= 0) //caso peggiore, trigger non trovato
          Serial.println("Segnale fuori portata!!!");
        else
        {
          rmsC = RMS(corrente,  campioniMisura);
          Serial.print("RMS corrente: ");
          Serial.print(rmsC);
		      Serial.println(" A");

          rmsV = RMS(tensione,  campioniMisura);
          Serial.print("RMS tensione: ");
          Serial.print(rmsV);
		      Serial.println(" V");

          P = potenzaAttiva(tensione, corrente,  campioniMisura);
          Serial.print("Potenza attiva: ");
          Serial.print(P);
		      Serial.println(" W");

          S = rmsV * rmsC;
          Serial.print("Potenza apparente: ");
          Serial.print(S);
		      Serial.println(" VA");

          Q = sqrt(constrain((S*S) - (P*P), 0, 1000000000));
          Serial.print("Potenza reattiva: ");
          Serial.print(Q);
		      Serial.println(" var");

          Serial.print("Fattore potenza: ");
          Serial.print(P/S);
		      Serial.println(" W/VA");
        }
      }

      Serial.print("\n\n\n");
      READY_FLAG=0;
    }
  
  
  delay(1);
}

void acquisisci(){

  /*tensione e corrente conterranno i valori reali in Volt e Ampere*/
  corrente[i]= ((float) analogRead(A0) - OFFSET_C ) * GAIN_C;
  tensione[i]= ((float) analogRead(A5) - OFFSET_V ) * GAIN_V;
  

  /*VECCHIO CODICE
  if(i<N-1)  
  {
    if(i==(N/2-1)) READY_FLAG = 1; //i=4999
    
    i++;
  }
  else //i=N
  {
    READY_FLAG = 2;
  
    i=0;
  }
  */
  
  i++;

  if (i == (N/2))		//sono arrivato a metà
  {
    if (READY_FLAG != 0)
		  OVERRUN_FLAG = 1;
    
    READY_FLAG = 1; //posso processare la prima metà
  }

  if (i >= N)		//sono arrivato alla fine
  { 
    i=0;
    
    if (READY_FLAG != 0)
		  OVERRUN_FLAG = 1;

    READY_FLAG = 2; //posso processare la seconda metà
  }
  
}


int triggerFirst(float arr[], float isteresi, int size)
{
  float sup = isteresi / 2;
  float inf = (-isteresi) / 2;

  int k=0;

  while((k < size) && (arr[k] > inf)) k++;

  while((k < size) && (arr[k] < sup)) k++;

  if(k==size)//sono arrivato al limite, restituisco -1
    return -1;
  else
    return k;
}

int triggerLast(float arr[], float isteresi, int size)
{
  float sup = isteresi / 2;
  float inf = (-isteresi) / 2;

  int k = size - 1;

  //cerco il primo campione a ritroso maggiore del livello 
  while((k >= 0) && (arr[k] < sup)) k--;
  
  //cerco il primo campione succesivo minore della soglia inferiore
  while((k > -1) && (arr[k] > inf)) k--; 

  if(k!=-1)
    while((k<size) && (arr[k]<sup)) k++;
  //se arriva a -1, cioè non trova nulla, restituisce proprio -1

  return k;
}

float potenzaAttiva(float tens[], float corr[], int numCampioni)
{
  float pot=0;

  for(int k=0; k<numCampioni; k++)
    pot+=tensione[k]*corrente[k];

  return pot/numCampioni;
}


float RMS(float v[], int numCampioni){
  float somma=0;

  for(int k = 0; k<numCampioni; k++)
    somma+= (v[k] * v[k]);

   return sqrt(somma / numCampioni);
}


float componenteContinua(float v[], int numCampioni){
  float somma=0;
  
  for(int k = 0; k<numCampioni; k++)
    somma+=v[k];    

   return somma / numCampioni;
}


void stampaVettore(float arr[], int dim)
{
  for(int k=0; k < dim; k++)
    Serial.println(arr[k]);
}