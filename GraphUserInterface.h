#pragma once

#include "Graph.h"

#include <QWidget>
#include <QVector>
#include <QPoint>
#include <QLabel>
#include <QTextEdit>


class GraphUserInterface : public QWidget {
    Q_OBJECT

public:
    GraphUserInterface(QWidget *parent = nullptr);

public slots:
    void updateGraphInfoLabel(bool isEmpty, int numVertices, int numEdges);
    void updateConsole(QString message);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    int findClickedVertex(const QPoint &position);

    void runBFS(int startVertexIndex);
    void runDFS(int startVertexIndex);
    void runDijkstra(int startVertexIndex, int endVertexIndex);
    void runAStar(int startVertexIndex, int endVertexIndex);

private slots:
    void onLoadClicked();
    void onSaveClicked();
    void onClearClicked();
    void onAbortClicked();

private:
    Graph graph;
    QSet<int> visitedVertices;
    QSet<QPair<int, int>> visitedEdges;

    int radius;

    QPoint dragOffset;
    int grabbedVertex;
    int firstSelectedVertex;

    int chosenAlgorithm;
    bool visualizationRunning;
    bool visualizationPaused;
    bool removingEdge;
    bool addingEdge;

    QLabel helpHints;
    QLabel graphInfoLabel;
    QTextEdit console;
    QVector<QLabel*> heuristicLabels;
};
