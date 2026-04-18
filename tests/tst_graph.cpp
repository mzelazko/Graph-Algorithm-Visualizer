#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QPoint>
#include <QPair>

#include "Graph.h"


class TestGraph : public QObject {
    Q_OBJECT

private slots:
    // addEdge: input validation
    void addEdge_selfLoop_isRejectedWithMessage();
    void addEdge_invalidIndex_isRejectedWithMessage();
    void addEdge_duplicateReversed_isRejectedWithMessage();

    // removeVertex: index shifting (the most important regression test)
    void removeVertex_shiftsIndicesInRemainingEdges();

    // save/load
    void saveLoad_roundTripPreservesGraph();
    void loadFromFile_skipsInvalidEdges();

    // Dijkstra vs A*
    void dijkstra_findsOptimalPathOverShortcut();
    void dijkstraVsAStar_agreeOnUniquePathInTree();
};


// Sums edge weights along a path returned by Dijkstra / A*.
// Looks the weights up in the original graph because the path only carries
// pairs of vertex indices.
static long long pathCost(const Graph& g,
                          const QVector<QPair<int, int>>& path) {
    long long total = 0;
    const auto& edges = g.getEdges();
    for (const auto& step : path) {
        for (const auto& e : edges) {
            const bool match =
                (e.first.first == step.first && e.first.second == step.second) ||
                (e.first.first == step.second && e.first.second == step.first);
            if (match) {
                total += e.second;
                break;
            }
        }
    }
    return total;
}


// ---------- addEdge: input validation ----------

void TestGraph::addEdge_selfLoop_isRejectedWithMessage() {
    Graph g;
    g.addVertex(QPoint(0, 0));

    QSignalSpy changedSpy(&g, &Graph::graphChanged);
    QSignalSpy msgSpy(&g, &Graph::graphMessage);

    g.addEdge(0, 0, 5);

    QCOMPARE(g.edgesSize(), 0);
    QCOMPARE(changedSpy.count(), 0);
    QCOMPARE(msgSpy.count(), 1);
    QVERIFY(msgSpy.takeFirst().at(0).toString().contains("Self-loops"));
}

void TestGraph::addEdge_invalidIndex_isRejectedWithMessage() {
    Graph g;
    g.addVertex(QPoint(0, 0));
    g.addVertex(QPoint(10, 0));

    QSignalSpy changedSpy(&g, &Graph::graphChanged);
    QSignalSpy msgSpy(&g, &Graph::graphMessage);

    g.addEdge(0, 5, 1);
    g.addEdge(-1, 0, 1);
    g.addEdge(2, 1, 1);

    QCOMPARE(g.edgesSize(), 0);
    QCOMPARE(changedSpy.count(), 0);
    QCOMPARE(msgSpy.count(), 3);
    QVERIFY(msgSpy.takeFirst().at(0).toString().contains("Invalid vertex index"));
}

void TestGraph::addEdge_duplicateReversed_isRejectedWithMessage() {
    Graph g;
    g.addVertex(QPoint(0, 0));
    g.addVertex(QPoint(10, 0));
    g.addEdge(0, 1, 5);

    QSignalSpy changedSpy(&g, &Graph::graphChanged);
    QSignalSpy msgSpy(&g, &Graph::graphMessage);

    g.addEdge(1, 0, 9); // reversed direction - graph is undirected

    QCOMPARE(g.edgesSize(), 1);
    QCOMPARE(g.getEdge(0).second, 5); // original weight is preserved
    QCOMPARE(changedSpy.count(), 0);
    QCOMPARE(msgSpy.count(), 1);
    QVERIFY(msgSpy.takeFirst().at(0).toString().contains("already exists"));
}


// ---------- removeVertex: index shifting ----------

void TestGraph::removeVertex_shiftsIndicesInRemainingEdges() {
    // Graph: vertices 0,1,2,3; edges (0,1), (1,3), (2,3).
    // Remove vertex 1:
    //   (0,1) -> incident to 1, removed
    //   (1,3) -> incident to 1, removed
    //   (2,3) -> both indices > 1, decremented -> (1,2), weight preserved
    Graph g;
    for (int i = 0; i < 4; ++i) {
        g.addVertex(QPoint(i * 10, 0));
    }
    g.addEdge(0, 1, 1);
    g.addEdge(1, 3, 2);
    g.addEdge(2, 3, 3);

    g.removeVertex(1);

    QCOMPARE(g.verticesSize(), 3);
    QCOMPARE(g.edgesSize(), 1);

    const auto edge = g.getEdge(0);
    QCOMPARE(edge.first.first, 1);
    QCOMPARE(edge.first.second, 2);
    QCOMPARE(edge.second, 3);
}


// ---------- save/load ----------

void TestGraph::saveLoad_roundTripPreservesGraph() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath("graph.txt");

    Graph original;
    original.addVertex(QPoint(0, 0));
    original.addVertex(QPoint(10, 5));
    original.addVertex(QPoint(20, 15));
    original.addEdge(0, 1, 4);
    original.addEdge(1, 2, 7);
    original.addEdge(0, 2, 3);

    original.saveToFile(path.toStdString());

    Graph loaded;
    loaded.loadFromFile(path.toStdString());

    QCOMPARE(loaded.verticesSize(), original.verticesSize());
    QCOMPARE(loaded.edgesSize(), original.edgesSize());

    for (int i = 0; i < original.verticesSize(); ++i) {
        QCOMPARE(loaded.getVertex(i), original.getVertex(i));
    }
    for (int i = 0; i < original.edgesSize(); ++i) {
        QCOMPARE(loaded.getEdge(i), original.getEdge(i));
    }
}

void TestGraph::loadFromFile_skipsInvalidEdges() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath("bad.txt");

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "0 0\n";
        out << "10 0\n";
        out << "EDGES\n";
        out << "0 0 5\n";       // self-loop -> skipped
        out << "0 5 5\n";       // index out of range -> skipped
        out << "0 1 4\n";       // valid
        out << "1 0 9\n";       // duplicate (reversed) -> skipped
        out << "\n";            // empty line -> skipped
    }

    Graph g;
    g.loadFromFile(path.toStdString());

    QCOMPARE(g.verticesSize(), 2);
    QCOMPARE(g.edgesSize(), 1);
    QCOMPARE(g.getEdge(0).first.first, 0);
    QCOMPARE(g.getEdge(0).first.second, 1);
    QCOMPARE(g.getEdge(0).second, 4);
}


// ---------- Dijkstra vs A* ----------

void TestGraph::dijkstra_findsOptimalPathOverShortcut() {
    // Triangle: the direct edge 0-2 is expensive (weight 10),
    // but the detour 0-1-2 has total weight 2 and should be the answer.
    Graph g;
    g.addVertex(QPoint(0, 0));
    g.addVertex(QPoint(1, 0));
    g.addVertex(QPoint(2, 0));
    g.addEdge(0, 1, 1);
    g.addEdge(1, 2, 1);
    g.addEdge(0, 2, 10);

    const auto result = g.dijkstra(0, 2);
    const auto& shortest = result.second;

    QCOMPARE(shortest.size(), 2);
    QCOMPARE(shortest[0], qMakePair(0, 1));
    QCOMPARE(shortest[1], qMakePair(1, 2));
    QCOMPARE(pathCost(g, shortest), 2LL);
}

void TestGraph::dijkstraVsAStar_agreeOnUniquePathInTree() {
    // In a tree there is exactly one path between any two vertices, so
    // regardless of whether the heuristic is admissible, Dijkstra and A*
    // must return the same shortestPath. This removes the dependency on
    // screen size (heuristicScaleDivisor() uses QGuiApplication::primaryScreen()).
    Graph g;
    g.addVertex(QPoint(0, 0));    // 0
    g.addVertex(QPoint(10, 0));   // 1
    g.addVertex(QPoint(20, 0));   // 2
    g.addVertex(QPoint(0, 10));   // 3
    g.addVertex(QPoint(0, 20));   // 4
    g.addEdge(0, 1, 3);
    g.addEdge(1, 2, 5);
    g.addEdge(0, 3, 2);
    g.addEdge(3, 4, 7);

    const auto dij = g.dijkstra(0, 2);
    const auto ast = g.aStar(0, 2);

    QCOMPARE(dij.second, ast.second);
    QCOMPARE(pathCost(g, dij.second), pathCost(g, ast.second));
    QCOMPARE(pathCost(g, dij.second), 8LL); // 0->1->2 = 3+5
}


QTEST_MAIN(TestGraph)
#include "tst_graph.moc"
