#pragma once

#include <QPoint>
#include <QVector>
#include <QObject>


class Graph: public QObject {
    Q_OBJECT

public:
    void clear();

    void addVertex(const QPoint position);
    void addEdge(int v1Index, int v2Index, int weight);
    void removeVertex(int vIndex);
    void removeEdge(int v1Index, int v2Index);
    void changeVertexPosition(int vIndex, const QPoint& newPosition);
    void saveToFile(const std::string &filename);
    void loadFromFile(const std::string &filename);

    int verticesSize() const;
    int edgesSize() const;
    const QVector<QPoint>& getVertices() const;
    const QVector<QPair<QPair<int, int>, int>>& getEdges() const;
    const QPoint getVertex(int index) const;
    const QPair<QPair<int, int>, int> getEdge(int index) const;
    double euclideanDistance(int v1Index, int v2Index);

    static int heuristicScaleDivisor();

    QVector<QPair<int, int>> bfs(int start);
    QVector<QPair<int, int>> dfs(int start);
    QPair<QVector<QPair<int, int>>, QVector<QPair<int, int>>> dijkstra(int start, int end);
    QPair<QVector<QPair<int, int>>, QVector<QPair<int, int>>> aStar(int start, int end);

signals:
    void graphChanged(bool isEmpty, int vertices, int edges);
    void graphMessage(QString message);

private:
    bool vertexInRange(int index) const;

    QVector<QPoint> vertices;
    QVector<QPair<QPair<int, int>, int>> edges;

};
