#include "burnupcalc.h"

using namespace std;

double intpol(double y0, double y1, double x0, double x1, double x) {
    // linear interpolation function
    double y = y0 + (y1 - y0)*(x - x0)/(x1 - x0);
    return y;
}



// need to add units

map<int, double> tomass (int ti, double time, isoInformation isoinfo) {
    map<int, double> out = map<int, double>();
    double mass_i;
    string name_i;
    int nucid;
    for (int i = 0; i < isoinfo.iso_vector.size(); i++){
        name_i = isoinfo.iso_vector[i].name;
        nucid = pyne::nucname::zzaaam(name_i)/10;
        mass_i = intpol(isoinfo.iso_vector[i].mass[ti-1],
                        isoinfo.iso_vector[i].mass[ti],
                        isoinfo.time[ti-1],
                        isoinfo.time[ti],
                        time);
        out[nucid] = mass_i;
    }
    return out;
}

/**
phicalc calculates the flux of a given batch
inputs: n is the batch number starting from zero, total bathces N, discharge burnup BU_total
        isoInformation of the fuel
**/

double phicalc(int n, int N, double BU_total, isoInformation tempone){
    int i = 0;
    double x0=0, x1, F_n, dF_n, dB_n, dt_n;
    double BU_n;
    cout<< "BU total: "<<BU_total<<endl;
    BU_n = BU_total*(n+1)/N;

    while (x0 + tempone.BUd[i] < BU_n) // finds the discrete point i corresponding to the burnup just under BU_n
        {
            x0 += tempone.BUd[i]; //x0 is the total burnup under BU_n
            i++;
        }

    x1 = x0 + tempone.BUd[i]; // adds on more discrete point for linear interpolation

    F_n = intpol(tempone.time[i-1],tempone.time[i],x0,x1,BU_n); //fluence at BU_n
cout<<"Fn: "<< F_n<<" time:"<<tempone.time[i]<<endl;
cout<<"x1: "<<x1<<" BU_n: "<<BU_n<<endl;
    dF_n = (tempone.time[i] - F_n); //delta fluence
    dB_n = x1 - BU_n; //delta burnup due to the delta fluence
    dt_n = (N * dB_n*180) / BU_total; //delta time due to the change in fluence
cout<< "dF_n: "<< dF_n<<" dB_n: "<<dB_n<<" dt_n: "<<dt_n<<endl;
    return dF_n/dt_n; //flux of n'th batch, n indexed from zero

}


double kcalc(isoInformation tempone, double BU_total, int N, double s_contr[3]){
    double x0 = 0, x1 = 0, BU_n = 0, phi, pbatch[N], dbatch[N], p_total=0, d_total=0;
    int j = 0, i = 0;

cout<<endl;
            for (j = 0; j < N; j++) {

                BU_n = BU_total*(j+1)/N;

                i=0;
                while (x0 + tempone.BUd[i] < BU_n) // finds the discrete point i corresponding to the burnup just under BU_n
                    {
                        x0 += tempone.BUd[i];
                        i++;
                    }

                x1 = x0 + tempone.BUd[i]; // adds on more discrete point for linear interpolation

                phi = phicalc(j, N, BU_total, tempone);
                cout<< "batch"<<j+1<<" phi: "<<phi<< endl;
                pbatch[j] = intpol(tempone.neutron_prod[i-1],tempone.neutron_prod[i], x0, x1, BU_n);
                dbatch[j] = intpol(tempone.neutron_dest[i-1],tempone.neutron_dest[i], x0, x1, BU_n);

                p_total += pbatch[j]*phi;
                d_total += dbatch[j]*phi;

                x0 = 0;
                x1= 0;

            }

        return s_contr[2]*(p_total+s_contr[0])/(d_total+s_contr[1]);

}


/**

burnupcalc takes a reactor-database-vector, batch number,
and tolerance; and finds the achievable burnup of the fuel.
The passed database vector is specific to a reactor and
contains neutron production, destruction, time, burnup,
and isotopic information. burnupcalc initially finds the
k-value of the fuel at time-step, and will return the total
burnup when k=1 in the case of a single batch loading. If
there is more than one batch, burnupcalc initially estimates
the burnup using a linear approximation and checks the
validity of the guess until the core average k-value is
within 0.0001. The function returns this burnup.


Inputs

tempone: This variable is of type isoInformation, and is a
reactor-specific database. It contains neutron production,
neutron destruction, burnup, time, and isotope vectors. The
variable type also contains an empty vector for k-infinity,
which is filled by burnupcalc in accordance with additional
assumptions (such as neutron leakage, which is subtracted
from neutron production).

N: Number of batches used in the core. Used to account for
batch loading and burnup calculations.

tolerance: Currently not utilized, used to adjust precision.


Outputs

Double, total burnup reached by the core.
*/

pair<double, map<int, double> > burnupcalc(isoInformation tempone, int N, double tolerance) {
    pair<double, map<int,double> > rtn(0, map<int, double>());
    double time_f; // time when k reaches one, time in days
    double BU_total = 0;
    double k_total = 10;
    int i = 0;
    double BU1, BU2, BU3;

    //read the structural material contribution information (production[0],
    //destruction[1], leakage[2]) to s_contr
    double s_contr[3];
    ifstream fin("../Bright-lite/LWR/LWRSTRUCT.txt");
    double passer;
    string spasser;
	while(i < 3)
	{
        fin >> spasser >> passer;
        s_contr[i]=passer;
        i++;
	}

	i = 0;
    while (i < tempone.neutron_prod.size())
    {
        tempone.k_inf.push_back(((tempone.neutron_prod[i]+s_contr[0])*s_contr[2])/(tempone.neutron_dest[i]+s_contr[1]));
        //tempone.k_inf.push_back(((tempone.neutron_prod[i]))/(tempone.neutron_dest[i]));
        i++;
    }

    i=0;
    while (tempone.k_inf[i] > 1.0){
        BU_total += tempone.BUd[i];
        i++; // finds the number of entry when k drops under 1

    }

    BU_total = intpol(BU_total,BU_total+tempone.BUd[i],tempone.k_inf[i-1],tempone.k_inf[i],1.0);
    time_f = intpol(tempone.time[i-1],tempone.time[i],tempone.k_inf[i-1],tempone.k_inf[i],1.0);


/*
    if (N == 1){
        rtn.first = BU_total;
        rtn.second = tomass(i, time_f, tempone);
        return rtn;
    }
*/


    BU1 = 2.* N * BU_total/(N+1);
    BU2 = BU1*1.1;
    k_total = 2;
    i=0;


        while(abs(1-k_total)>0.000001){

            BU3 = BU2 - (kcalc(tempone, BU2, N, s_contr)-1)*(BU2-BU1)/((kcalc(tempone, BU2, N, s_contr))-(kcalc(tempone, BU1, N, s_contr)));
            k_total = kcalc(tempone, BU3, N, s_contr);
            BU1 = BU2;
            BU2 = BU3;
            i++;

            if(i==50)
                {
                cout<< "Warning! Maximum iteration reached."<<endl;
                BU3 = (BU1+BU2+BU3)/3;
                break;
                }



        }

    BU_total = BU3;


    rtn.first = BU_total;
    rtn.second = tomass(i, time_f, tempone);
    return rtn;

}


/**
enrichcalc takes the burnup goal, batch number, and tolerance;
and returns the enrichment needed to reach the burnup goal.
The code initially estimates the enrichment needed by
calculating the burnup (for a given number of batches and type
of reactor) of a two and seven percent enriched fuel and then
interpolates/extrapolates from these two points. The code then
calculates the burnup for this estimated enrichment level, and
iterates by changing the guess enrichment until the desired
burnup is achieved by the guess value. This enrichment is then
returned.


Inputs

BU_end: The target burnup. The code finds an enrichment which
will result in a burnup equal to this value.

N: number of batches used in the core.

tolerance: Currently not utilized, used to adjust precision.


Outputs:

Double, the enrichment needed to achieve BU_end.

*/


double enrichcalc(double BU_end, int N, double tolerance, int type, vector<isoInformation> input_stream)
{

double X;
double BU_guess;
double BU2, BU7;
isoInformation test2;


// accurate guess calc, extrapolating from two data points at 2 and 7% enrichment
BU2 = 20;
BU7 = 100;

X = 0.02 + (BU_end - BU2)*(0.07 - 0.02)/(BU7 - BU2);

BU_guess = burnupcalc(DataReader(test2, type, input_stream), N, 0.01).first;

// enrichment iteration
while (BU_end < BU_guess)
{
    X = X - 0.001;
    BU_guess = burnupcalc(DataReader(test2, type, input_stream), N, 0.01 ).first;
}


while (BU_end > BU_guess)
{
    X = X + 0.001;
    BU_guess = burnupcalc(DataReader(test2, type, input_stream), N, 0.01 ).first;
}
    return X;


}

int main(){
    NonActinideReader("PWRU50.LIB");
    isoInformation testVector;
    double BUd_sum = 0;
    int N;
    double X;
    isoInformation test1;
    vector<isoInformation> input_stream;
    ifstream inf("../Bright-lite/inputFile.txt");
    string line;
    double mass_total;
    while (getline(inf, line)) {
        isoInformation temp_iso;
        istringstream iss(line);
        iss >> temp_iso.name;
        iss >> temp_iso.fraction;
        mass_total = mass_total + temp_iso.fraction;
        input_stream.push_back(temp_iso);
    }
    for (int i = 0; i < input_stream.size(); i++){
        input_stream[i].fraction = input_stream[i].fraction / mass_total;
        cout << input_stream[i].name << " " << input_stream[i].fraction << endl;
    }
    double enrichment = input_stream[0].fraction;
    inf.close();
    map<int, double> test_mass;
    map<int, double>::iterator Iter;
    double BU_end;
    double BU_d;


 //   BU_d = burnupcalc(DataReader(test1, 1, input_stream), 2, .01).first;

  //  cout << "N=1: "<< burnupcalc(DataReader(test1, 1, input_stream), 1, .01).first << endl<< endl;
    cout << "N=2: "<< burnupcalc(DataReader(test1, 1, input_stream), 2, .01).first << endl<< endl;
 //   cout << "N=3: "<< burnupcalc(DataReader(test1, 1, input_stream), 3, .01).first << endl<< endl;
 //   cout << "N=5: "<< burnupcalc(DataReader(test1, 1, input_stream), 5, .01).first << endl<< endl;
 //   cout << "N=10: "<< burnupcalc(DataReader(test1, 1, input_stream), 10, .01).first << endl;
 //   cout << "N=1000: "<< burnupcalc(DataReader(test1, 1, input_stream), 1000, .01).first << endl;


//    test_mass = burnupcalc(DataReader(test1, 1, input_stream), 3, .01).second;
 //   cout << "Burnup is  " << BU_d << endl;
    /*for (Iter = test_mass.begin(); Iter != test_mass.end(); ++Iter){
        string m = pyne::nucname::name((*Iter).first);
        if ((*Iter).second > 0.01){
            outf2 << m << "   " << (*Iter).second << endl;
            /** STUPID UGLY UGLY CODE
            if (m == "Am241" || m == "Am243" || m == "Cm242" || m == "Cm244" || m == "Np237" || m == "Np238" || m == "Np239"){
                outf1 << m << "  " << (*Iter).second << endl;
            }
            if (m == "Pu238" || m == "Pu239" || m == "Pu240" || m == "Pu241" || m == "Pu242" || m == "U234" || m == "U235"){
                outf1 << m << "  " << (*Iter).second << endl;
            }
            if (m == "U236" || m == "U237" || m == "U238"){
                outf1 << m << "  " << (*Iter).second << endl;
            }
        }
    }*/

    return 0;
}



