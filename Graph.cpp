#include "Graph.h"

#include <QQueue>
#include <QStack>
#include <fstream>
#include <sstream>
#include <queue>
#include <limits>
#include <cmath>
#include <QGuiApplication>
#include <QScreen>
#include <QtGlobal>


bool Graph::vertexInRange(int index) const {
    return index >= 0 && index < vertices.size();
}

int Graph::heuristicScaleDivisor() {
    const QScreen *screen = QGuiApplication::primaryScreen();
    const int w = screen ? screen->geometry().width() : 0;
    return qMax(1, w);
}

void Graph::clear() {
    vertices.clear();
    edges.clear();
    emit graphChanged(vertices.empty(), vertices.size(), edges.size());
}

void Graph::addVertex(const QPoint position) {
    vertices.push_back(position);
    emit graphChanged(vertices.empty(), vertices.size(), edges.size());
}

void Graph::addEdge(int v1Index, int v2Index, int weight) {

    if (!vertexInRange(v1Index) || !vertexInRange(v2Index)) {
        emit graphMessage("Invalid vertex index — action cancelled.");
        return;
    }
    if(v1Index == v2Index){
        emit graphMessage("Self-loops are not allowed — action cancelled.");
        return;
    }
    for(int i = 0; i < edges.size(); i++){

        if(edges[i].first == QPair<int,int>(v1Index,v2Index) || edges[i].first == QPair<int,int>(v2Index,v1Index)){
            emit graphMessage("An edge between these vertices already exists — action cancelled.");
            return;
        }
    }
    edges.push_back({{v1Index, v2Index}, weight});

    emit graphChanged(vertices.empty(), vertices.size(), edges.size());
}

void Graph::removeVertex(int vIndex){
    if (!vertexInRange(vIndex)) {
        emit graphMessage("Invalid vertex index — action cancelled.");
        return;
    }
    for (int i = edges.size() - 1; i >= 0; --i) {
        if (edges[i].first.first == vIndex || edges[i].first.second == vIndex) {
            edges.remove(i);
        }

        // Removing a vertex shifts indices in QVector; edge endpoints above the removed index must be decremented.
        else {
            if (edges[i].first.first > vIndex) {
                edges[i].first.first--;
            }
            if (edges[i].first.second > vIndex) {
                edges[i].first.second--;
            }
        }
    }
    vertices.remove(vIndex);

    emit graphChanged(vertices.empty(), vertices.size(), edges.size());
}

void Graph::removeEdge(int v1Index, int v2Index){
    if (!vertexInRange(v1Index) || !vertexInRange(v2Index)) {
        emit graphMessage("Invalid vertex index — action cancelled.");
        return;
    }
    bool edgeFound = 0;

    for (int i = edges.size() - 1; i >= 0; --i) {
        if (edges[i].first.first == v1Index && edges[i].first.second == v2Index) {
            edges.remove(i);
            edgeFound = 1;
            break;
        }
        else if(edges[i].first.first == v2Index && edges[i].first.second == v1Index){
            edges.remove(i);
            edgeFound = 1;
            break;
        }
    }

    if(!edgeFound){
        emit graphMessage("No edge between these vertices — removal cancelled.");
        return;
    }

    emit graphChanged(vertices.empty(), vertices.size(), edges.size());
}

void Graph::changeVertexPosition(int vIndex, const QPoint& newPosition) {
    if (!vertexInRange(vIndex)) {
        return;
    }
    vertices[vIndex] = newPosition;
}


void Graph::saveToFile(const std::string &filename) {
    std::ofstream outfile(filename);
    if (!outfile) {
        return;
    }
    for (const QPoint &vertex : vertices) {
        outfile << vertex.x() << " " << vertex.y() << "\n";
    }
    outfile << "EDGES\n";
    for (const QPair<QPair<int, int>, int> &edge : edges) {
        outfile << edge.first.first << " " << edge.first.second << " "<< edge.second << "\n";
    }
}

void Graph::loadFromFile(const std::string &filename) {
    std::ifstream infile(filename);
    if (!infile) {
        return;
    }

    vertices.clear();
    edges.clear();

    std::string line;
    while (std::getline(infile, line)) {
        if (line == "EDGES") {
            break;
        }

        std::stringstream ss(line);
        int x, y;
        ss >> x >> y;
        vertices.push_back(QPoint(x, y));
    }
    while (std::getline(infile, line)) {
        if (line.empty()) {
            continue;
        }
        std::stringstream ss(line);
        int v1, v2, w;
        ss >> v1 >> v2 >> w;
        if (ss.fail()) {
            continue;
        }
        if (v1 == v2 || !vertexInRange(v1) || !vertexInRange(v2)) {
            continue;
        }
        bool duplicate = false;
        for (int i = 0; i < edges.size(); i++) {
            if (edges[i].first == QPair<int, int>(v1, v2) || edges[i].first == QPair<int, int>(v2, v1)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            edges.push_back({{v1, v2}, w});
        }
    }

    emit graphChanged(vertices.empty(), vertices.size(), edges.size());
}


int Graph::verticesSize() const {
    return vertices.size();
}

int Graph::edgesSize() const {
    return edges.size();
}

const QVector<QPoint>& Graph::getVertices() const {
    return vertices;
}

const QVector<QPair<QPair<int, int>, int>>& Graph::getEdges() const {
    return edges;
}

const QPoint Graph::getVertex(int index) const {
    if (!vertexInRange(index)) {
        return QPoint();
    }
    return vertices[index];
}

const QPair<QPair<int, int>, int> Graph::getEdge(int index) const {
    if (index < 0 || index >= edges.size()) {
        return QPair<QPair<int, int>, int>(QPair<int, int>(-1, -1), 0);
    }
    return edges[index];
}

double Graph::euclideanDistance(int v1Index, int v2Index) {
    if (!vertexInRange(v1Index) || !vertexInRange(v2Index)) {
        return 0.0;
    }
    const QPointF v1 = vertices[v1Index];
    const QPointF v2 = vertices[v2Index];
    return std::sqrt(std::pow(v1.x() - v2.x(), 2) + std::pow(v1.y() - v2.y(), 2));
}

QVector<QPair<int, int>> Graph::bfs(int start) {
    QVector<QPair<int, int>> path;

    if (!vertexInRange(start)) {
        return path;
    }

    QVector<bool> visited(vertices.size(), false);
    QQueue<int> queue;

    visited[start] = true;
    queue.enqueue(start);

    while (!queue.isEmpty()) {
         int current = queue.dequeue();

         for (const QPair<QPair<int, int>, int> &edge : edges) {
            int neighbour;
            if (edge.first.first == current) {
                neighbour = edge.first.second;
            } else if (edge.first.second == current) {
                neighbour = edge.first.first;
            }
            else{
                continue;
            }
            if (!visited[neighbour]) {
                visited[neighbour] = true;
                queue.enqueue(neighbour);
                path.push_back({current, neighbour});
            }
         }
    }
    return path;
}

QVector<QPair<int, int>> Graph::dfs(int start) {
    QVector<QPair<int, int>> path;

    if (!vertexInRange(start)) {
        return path;
    }

    QVector<bool> visited(vertices.size(), false);

    QStack<QPair<int, int>> stack;

    stack.push({-1, start});

    while (!stack.isEmpty()) {
         int current = stack.top().second;
         int previous = stack.top().first;
         stack.pop();
         if (visited[current]) {
            continue;
         }
         visited[current] = true;

         if(previous != -1) {
            path.push_back({previous, current});
         }

         for (const QPair<QPair<int, int>, int> &edge : edges) {
            int neighbour;
            if (edge.first.first == current) {
                neighbour = edge.first.second;
            } else if (edge.first.second == current) {
                neighbour = edge.first.first;
            }
            else{
                continue;
            }
            if (!visited[neighbour]) {
                stack.push({current, neighbour});
            }
         }
    }
    return path;
}

QPair<QVector<QPair<int, int>>, QVector<QPair<int, int>>> Graph::dijkstra(int start, int end) {
    QPair<QVector<QPair<int, int>>, QVector<QPair<int, int>>> emptyResult;
    if (!vertexInRange(start) || !vertexInRange(end)) {
        return emptyResult;
    }

    const auto kInf = std::numeric_limits<long long>::max();
    QVector<long long> cost(vertices.size(), kInf);
    QVector<bool> visited(vertices.size(), false);

    std::priority_queue<std::pair<long long, int>, std::vector<std::pair<long long, int>>, std::greater<std::pair<long long, int>>> pqueue;
    QVector<int> prev(vertices.size(), -1);
    cost[start] = 0;
    pqueue.push({0, start});

    QVector<QPair<int, int>> fullPath;

    while (!pqueue.empty()) {
        int current = pqueue.top().second;
        pqueue.pop();

        if (visited[current]) {
            continue;
        }

        if (prev[current] != -1) {
            fullPath.push_back({prev[current], current});
        }

        if (current == end) {
            break;
        }

        visited[current] = true;

        for (const QPair<QPair<int, int>, int> &edge : edges) {
            int neighbour, weight;
            if (edge.first.first == current) {
                neighbour = edge.first.second;
                weight = edge.second;
            } else if (edge.first.second == current) {
                neighbour = edge.first.first;
                weight = edge.second;
            } else {
                continue;
            }
            if (cost[current] == kInf) {
                continue;
            }
            const long long relaxed = cost[current] + static_cast<long long>(weight);
            if (relaxed < cost[neighbour]) {
                cost[neighbour] = relaxed;
                prev[neighbour] = current;
                pqueue.push({cost[neighbour], neighbour});
            }
        }
    }

    QVector<QPair<int, int>> shortestPath;
    int current = end;
    while (current != -1) {
        int previous = prev[current];
        if (previous != -1) {
            shortestPath.push_front({previous, current});
        }
        current = previous;
    }
    return {fullPath, shortestPath};
}

QPair<QVector<QPair<int, int>>, QVector<QPair<int, int>>> Graph::aStar(int start, int end) {
    QPair<QVector<QPair<int, int>>, QVector<QPair<int, int>>> emptyResult;
    if (!vertexInRange(start) || !vertexInRange(end)) {
        return emptyResult;
    }

    const auto kInf = std::numeric_limits<long long>::max();
    QVector<long long> partialCost(vertices.size(), kInf);
    QVector<double> fullCost(vertices.size(), std::numeric_limits<double>::max());
    QVector<int> prev(vertices.size(), -1);
    QVector<bool> visited(vertices.size(), false);

    std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, std::greater<std::pair<double, int>>> pqueue;
    QVector<QPair<int, int>> fullPath;

    const int denom = heuristicScaleDivisor();
    partialCost[start] = 0;
    fullCost[start] = 20.0 * euclideanDistance(start, end) / static_cast<double>(denom);
    pqueue.push({fullCost[start], start});

    while (!pqueue.empty()) {
        int current = pqueue.top().second;
        pqueue.pop();

        if (visited[current]) continue;

        if (prev[current] != -1) {
            fullPath.push_back({prev[current], current});
        }

        if (current == end) {
            break;
        }

        visited[current] = true;
        for (const QPair<QPair<int, int>, int> &edge : edges) {
            int neighbour, weight;
            if (edge.first.first == current) {
                neighbour = edge.first.second;
                weight = edge.second;
            } else if (edge.first.second == current) {
                neighbour = edge.first.first;
                weight = edge.second;
            }
            else{
                continue;
            }
            if (!visited[neighbour]) {
                if (partialCost[current] == kInf) {
                    continue;
                }
                const long long pCost = partialCost[current] + static_cast<long long>(weight);
                if (pCost < partialCost[neighbour]) {
                    prev[neighbour] = current;
                    partialCost[neighbour] = pCost;
                    fullCost[neighbour] = static_cast<double>(pCost) + 20.0 * euclideanDistance(neighbour, end) / static_cast<double>(denom);
                    pqueue.push({fullCost[neighbour], neighbour});
                }
            }
        }
    }

    QVector<QPair<int, int>> shortestPath;
    int current = end;
    while (current != -1) {
        int previous = prev[current];
        if (previous != -1) {
            shortestPath.push_front({previous, current});
        }
        current = previous;
    }
    return {fullPath, shortestPath};
}


