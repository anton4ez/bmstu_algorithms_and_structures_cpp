#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <random>
#include <limits>
#include <stdexcept>
#include <iomanip>
#include <cmath>

using namespace std;

struct Graph {
    int n = 0;
    vector<string> names;
    vector<vector<double>> dist;

    bool loadFromFile(const string& filename) {
        ifstream fin(filename);
        if (!fin.is_open()) {
            cerr << "Не удалось открыть файл: " << filename << "\n";
            return false;
        }

        string line;
        if (!getline(fin, line)) {
            cerr << "Файл пустой\n";
            return false;
        }

        istringstream iss(line);
        string city;
        names.clear();
        while (iss >> city) {
            names.push_back(city);
        }

        n = (int)names.size();
        if (n == 0) {
            cerr << "В первой строке нет названий городов\n";
            return false;
        }

        dist.assign(n, vector<double>(n, 0.0));
        for (int i = 0; i < n; ++i) {
            if (!getline(fin, line)) {
                cerr << "Недостаточно строк для матрицы расстояний\n";
                return false;
            }
            istringstream row(line);
            for (int j = 0; j < n; ++j) {
                if (!(row >> dist[i][j])) {
                    cerr << "Ошибка чтения элемента d[" << i << "][" << j << "]\n";
                    return false;
                }
            }
        }
        return true;
    }
};

struct Ant {
    vector<int> tour;
    vector<bool> visited;
    double length = 0.0;
};

double computeQ(const Graph& graph) {
    double sum = 0.0;
    int n = graph.n;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            sum += graph.dist[i][j];
        }
    }
    return sum / (n - 1);
}

double tourLength(const Graph& graph, const vector<int>& tour) {
    double L = 0.0;
    int n = graph.n;
    for (int i = 0; i < n - 1; i++) {
        L += graph.dist[tour[i]][tour[i + 1]];
    }
    L += graph.dist[tour.back()][tour.front()];
    return L;
}

void runAntColony(const Graph& graph, int numAnts, int maxIter, double alpha,
                  double beta, double rho, vector<int>& bestTour, double& bestLength,
                  bool printProgress = true) {
    int n = graph.n;
    const double tau0 = 1.0;

    mt19937 gen(random_device{}());
    uniform_real_distribution<> urand(0.0, 1.0);
    uniform_int_distribution<> startDist(0, n - 1);

    vector<vector<double>> pheromone(n, vector<double>(n, tau0));
    double Q = computeQ(graph);

    bestLength = numeric_limits<double>::infinity();
    bestTour.clear();

    for (int it = 0; it < maxIter; ++it) {
        vector<Ant> ants(numAnts);

        for (int k = 0; k < numAnts; ++k) {
            Ant& ant = ants[k];
            ant.tour.assign(n, -1);
            ant.visited.assign(n, false);

            int start = startDist(gen);
            ant.tour[0] = start;
            ant.visited[start] = true;

            for (int step = 1; step < n; ++step) {
                int current = ant.tour[step - 1];
                vector<double> probs(n, 0.0);
                double sumProb = 0.0;

                for (int j = 0; j < n; ++j) {
                    if (!ant.visited[j]) {
                        double tau = pheromone[current][j];
                        double distance = graph.dist[current][j];
                        double eta = (distance == 0.0) ? 1e9 : 1.0 / distance; 
                        
                        double val = pow(tau, alpha) * pow(eta, beta);
                        probs[j] = val;
                        sumProb += val;
                    }
                }

                double r = urand(gen) * sumProb;
                double acc = 0.0;
                int next = -1;

                for (int j = 0; j < n; j++) {
                    if (!ant.visited[j]) {
                        acc += probs[j];
                        if (acc >= r) {
                            next = j;
                            break;
                        }
                    }
                }

                if (next == -1) {
                    for (int j = 0; j < n; j++) {
                        if (!ant.visited[j]) {
                            next = j;
                            break;
                        }
                    }
                }

                ant.tour[step] = next;
                ant.visited[next] = true;
            }

            ant.length = tourLength(graph, ant.tour);
            if (ant.length < bestLength) {
                bestLength = ant.length;
                bestTour = ant.tour;
            }
        }

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                pheromone[i][j] *= (1.0 - rho);
            }
        }

        for (int k = 0; k < numAnts; k++) {
            const Ant& ant = ants[k];
            double delta = Q / ant.length;
            for (int idx = 0; idx < n - 1; idx++) {
                int i = ant.tour[idx];
                int j = ant.tour[idx + 1];
                pheromone[i][j] += delta;
                pheromone[j][i] += delta;
            }
            int last = ant.tour[n - 1];
            int first = ant.tour[0];
            pheromone[last][first] += delta;
            pheromone[first][last] += delta;
        }

        if (printProgress && ((it + 1) % 50 == 0 || it == maxIter - 1)) {
            cout << "Итерация " << (it + 1) << ", текущая лучшая длина = " << bestLength << " (в сотнях км)\n";
        }
    }
}

void bruteForceDFS(const Graph& graph, int startCity, vector<bool>& used,
                   vector<int>& currentTour, int depth, double currentLength, 
                   vector<int>& bestTour, double& bestLength) {
    int n = graph.n;

    if (depth == n) {
        int last = currentTour[depth - 1];
        double total = currentLength + graph.dist[last][startCity];
        if (total < bestLength) {
            bestLength = total;
            bestTour = currentTour;
        }
        return;
    }

    int lastCity = currentTour[depth - 1];
    for (int city = 0; city < n; city++) {
        if (used[city]) continue;

        used[city] = true;
        currentTour[depth] = city;
        double newLen = currentLength + graph.dist[lastCity][city];
        
        bruteForceDFS(graph, startCity, used, currentTour, depth + 1, newLen, bestTour, bestLength);
        
        used[city] = false;
    }
}

void runBruteForceTSP(const Graph& graph, int startCity, vector<int>& bestTour, double& bestLength) {
    int n = graph.n;
    bestLength = numeric_limits<double>::infinity();
    bestTour.assign(n, -1);
    
    vector<bool> used(n, false);
    vector<int> currentTour(n, -1);
    
    used[startCity] = true;
    currentTour[0] = startCity;
    
    bruteForceDFS(graph, startCity, used, currentTour, 1, 0.0, bestTour, bestLength);
}

int main() {
    setlocale(LC_ALL, "Russian");
    Graph graph;
    if (!graph.loadFromFile("cities.txt")) {
        cerr << "Ошибка загрузки файла cities.txt\n";
        return 1;
    }

    cout << "Загружено городов: " << graph.n << "\n\n";

    double alpha = 0.3;  
    double beta = 0.7;   
    double rho = 0.5;    
    int numAnts = 40;
    int maxIter = 1000;

    cout << "Запуск муравьиного алгоритма...\n";
    vector<int> bestAntTour;
    double bestAntLen;

    runAntColony(graph, numAnts, maxIter, alpha, beta, rho, bestAntTour, bestAntLen);

    cout << "\nЛучший маршрут (муравьиный алгоритм):\n";
    for (int i = 0; i < (int)bestAntTour.size(); i++) {
        cout << graph.names[bestAntTour[i]] << " -> ";
    }
    cout << graph.names[bestAntTour[0]] << "\n";
    cout << fixed << setprecision(2);
    cout << "Длина цикла (муравьиный алгоритм): " << bestAntLen 
         << " (в сотнях км) = " << bestAntLen * 100 << " км\n\n";

    char choice;
    cout << "Выполнить решение задачи полным перебором? (y/n): ";
    if (!(cin >> choice)) return 0;

    if (choice == 'y' || choice == 'Y') {
        const int maxBruteN = 12;
        if (graph.n > maxBruteN) {
            cout << "\nПредупреждение: n = " << graph.n 
                 << ". Полный перебор выполним только на подграфе из " << maxBruteN << " первых городов.\n\n";
            
            Graph small;
            small.n = maxBruteN;
            small.names.assign(graph.names.begin(), graph.names.begin() + maxBruteN);
            small.dist.assign(maxBruteN, vector<double>(maxBruteN, 0.0));
            
            for (int i = 0; i < maxBruteN; i++) {
                for (int j = 0; j < maxBruteN; j++) {
                    small.dist[i][j] = graph.dist[i][j];
                }
            }

            cout << "Запуск полного перебора на подграфе...\n";
            vector<int> bestBFtour;
            double bestBFlen;
            runBruteForceTSP(small, 0, bestBFtour, bestBFlen);

            cout << "\nЛучший маршрут (полный перебор, подграф):\n";
            for (int i = 0; i < (int)bestBFtour.size(); i++) {
                cout << small.names[bestBFtour[i]] << " -> ";
            }
            cout << small.names[bestBFtour[0]] << "\n";
            cout << "Длина цикла (полный перебор, подграф): " << bestBFlen 
                 << " (в сотнях км) = " << bestBFlen * 100 << " км\n";
        } else {
            cout << "\nЗапуск полного перебора на всём графе...\n";
            vector<int> bestBFtour;
            double bestBFlen;
            runBruteForceTSP(graph, 0, bestBFtour, bestBFlen);

            cout << "\nЛучший маршрут (полный перебор):\n";
            for (int i = 0; i < (int)bestBFtour.size(); i++) {
                cout << graph.names[bestBFtour[i]] << " -> ";
            }
            cout << graph.names[bestBFtour[0]] << "\n";
            cout << "Длина цикла (полный перебор): " << bestBFlen 
                 << " (в сотнях км) = " << bestBFlen * 100 << " км\n";
        }
    }
    return 0;
}
