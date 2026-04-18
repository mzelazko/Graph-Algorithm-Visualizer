#include "GraphUserInterface.h"
#include "Graph.h"

#include <QApplication>
#include <QPushButton>
#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QQueue>
#include <QTimer>
#include <QSet>
#include <QStack>
#include <QFileDialog>
#include <QFont>
#include <QMenu>
#include <QMenuBar>
#include <QInputDialog>
#include <QSizePolicy>
#include <cmath>

namespace {
QString helpHintsText() {
    return QStringLiteral(
        "Controls:\n"
        "- New vertex: left-click empty space\n"
        "- New edge: Ctrl+left-click first vertex, then second; enter weight in the dialog.\n"
        "- Remove vertex: middle mouse button on the vertex.\n"
        "- Right-click vertex: BFS, DFS, Dijkstra, A*, Delete Edge.\n"
        "- Dijkstra / A*: after choosing the algorithm, right-click the target vertex."
    );
}

// Offset weight text perpendicular to the edge so labels do not stack at edge crossings (e.g. diagonals in K4).
QPointF edgeWeightLabelPos(const QPointF &v1, const QPointF &v2) {
    const QPointF mid = (v1 + v2) * 0.5;
    const qreal dx = v2.x() - v1.x();
    const qreal dy = v2.y() - v1.y();
    const qreal len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-5) {
        return mid;
    }
    constexpr qreal offset = 20.0;
    return mid + QPointF((-dy / len) * offset, (dx / len) * offset);
}
}

GraphUserInterface::GraphUserInterface(QWidget *parent) :
    QWidget(parent),
    radius(20),
    grabbedVertex(-1),
    firstSelectedVertex(-1),
    chosenAlgorithm(-1),
    visualizationRunning(0),
    visualizationPaused(0),
    removingEdge(0),
    addingEdge(0)
    {
    setMouseTracking(true);

    connect(&graph, &Graph::graphMessage, this, &GraphUserInterface::updateConsole);
    connect(&graph, &Graph::graphChanged, this, &GraphUserInterface::updateGraphInfoLabel);

    QPushButton *clearButton = new QPushButton("Clear", this);
    connect(clearButton, &QPushButton::clicked, this, &GraphUserInterface::onClearClicked);

    QPushButton *pauseVisualizationButton = new QPushButton("Pause visualization", this);
    connect(pauseVisualizationButton, &QPushButton::clicked, this, [this, pauseVisualizationButton] () {
        if (visualizationPaused) {
            visualizationPaused = 0;
            pauseVisualizationButton->setText("Pause visualization");
        } else {
            visualizationPaused = 1;
            pauseVisualizationButton->setText("Unpause visualization");
        }
    });

    QPushButton * abortVisualizationButton = new QPushButton("Abort visualization", this);
    connect(abortVisualizationButton, &QPushButton::clicked, this, &GraphUserInterface::onAbortClicked);

    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = menuBar->addMenu("File");

    QAction *loadAction = new QAction("Load Graph", this);
    connect(loadAction, &QAction::triggered, this, &GraphUserInterface::onLoadClicked);
    fileMenu->addAction(loadAction);

    QAction *saveAction = new QAction("Save Graph", this);
    connect(saveAction, &QAction::triggered, this, &GraphUserInterface::onSaveClicked);
    fileMenu->addAction(saveAction);

    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(exitAction);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMenuBar(menuBar);
    layout->addWidget(clearButton);
    layout->addWidget(abortVisualizationButton);
    layout->addWidget(pauseVisualizationButton);
    layout->addStretch();

    helpHints.setText(helpHintsText());
    helpHints.setWordWrap(true);
    helpHints.setTextInteractionFlags(Qt::NoTextInteraction);
    {
        QFont hf = helpHints.font();
        hf.setPointSize(8);
        helpHints.setFont(hf);
    }
    helpHints.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    helpHints.setContentsMargins(0, 0, 0, 0);
    helpHints.setAlignment(Qt::AlignLeft | Qt::AlignTop);

    graphInfoLabel.setAlignment(Qt::AlignRight | Qt::AlignBottom);
    QFont font = graphInfoLabel.font();
    font.setPointSize(10);
    graphInfoLabel.setFont(font);
    graphInfoLabel.setText("Empty: Yes\nNumber of vertices: 0\nNumber of edges: 0");

    console.setReadOnly(true);
    console.setFixedHeight(90);
    console.append(QStringLiteral("Console: messages and errors (e.g. duplicate edge, cancelled action)."));

    QWidget *bottomPanel = new QWidget(this);
    QVBoxLayout *bottomVBox = new QVBoxLayout(bottomPanel);
    bottomVBox->setContentsMargins(0, 0, 0, 0);
    bottomVBox->setSpacing(2);

    QGridLayout *helpStatsGrid = new QGridLayout();
    helpStatsGrid->setContentsMargins(0, 0, 0, 0);
    helpStatsGrid->setHorizontalSpacing(16);
    helpStatsGrid->setVerticalSpacing(0);
    helpStatsGrid->addWidget(&helpHints, 0, 0, Qt::AlignLeft | Qt::AlignBottom);
    helpStatsGrid->setColumnStretch(1, 1);
    helpStatsGrid->addWidget(&graphInfoLabel, 0, 2, Qt::AlignRight | Qt::AlignBottom);

    bottomVBox->addLayout(helpStatsGrid);
    bottomVBox->addWidget(&console);

    layout->addWidget(bottomPanel);
}

void GraphUserInterface::paintEvent(QPaintEvent *event){
    Q_UNUSED(event);
    QPainter painter(this);

    QFont font = painter.font();
    font.setPointSize(15);
    painter.setFont(font);

    for (const QPair<QPair<int, int>, int> &edge : graph.getEdges()) {
        if (visitedEdges.contains(edge.first) || visitedEdges.contains({edge.first.second, edge.first.first})) {
            painter.setPen(Qt::magenta);
        } else {
            painter.setPen(Qt::darkGray);
        }
        painter.drawLine(graph.getVertex(edge.first.first), graph.getVertex(edge.first.second));

        QPointF v1 = graph.getVertex(edge.first.first);
        QPointF v2 = graph.getVertex(edge.first.second);
        QPointF weightPos = edgeWeightLabelPos(v1, v2);

        QString weightText = QString::number(edge.second);
        painter.setPen(Qt::black);
        const QRectF textRect(weightPos.x() - 22, weightPos.y() - 11, 44, 22);
        painter.drawText(textRect, Qt::AlignCenter, weightText);
    }
    painter.setPen(Qt::black);
    for (int i = 0; i < graph.verticesSize(); i++) {
        painter.setBrush(visitedVertices.contains(i) ? Qt::green : Qt::red);
        painter.drawEllipse(graph.getVertex(i), radius, radius);
    }
}

void GraphUserInterface::mousePressEvent(QMouseEvent *event){
    int clickedVertex = findClickedVertex(event->pos());

    if(visualizationRunning == 0){

        if((addingEdge || removingEdge || chosenAlgorithm != -1) && clickedVertex == -1){
            console.append("Action cancelled — no second vertex selected.");
            firstSelectedVertex = -1;
            chosenAlgorithm = -1;
            removingEdge = 0;
            addingEdge = 0;
            return;
        }
        if((addingEdge && (event->button() != Qt::LeftButton || event->modifiers() != Qt::ControlModifier)) || ((removingEdge || chosenAlgorithm != -1) && event->button() != Qt::RightButton)){
            console.append("Action cancelled — wrong mouse button.");
            firstSelectedVertex = -1;
            chosenAlgorithm = -1;
            removingEdge = 0;
            addingEdge = 0;
            return;
        }

        if(event->button() == Qt::LeftButton && event->modifiers() == Qt::ControlModifier){
            if (clickedVertex != -1) {
                if (firstSelectedVertex == -1) {
                    firstSelectedVertex = clickedVertex;
                    addingEdge = 1;
                    console.append("Hold Ctrl and left-click the second vertex to add an edge.");
                } else {
                    bool ok;
                    int weight = QInputDialog::getInt(this, tr("Set edge weight"),tr("Weight:"), 1, 1, 21, 1, &ok);
                    if (ok) {
                        graph.addEdge(firstSelectedVertex, clickedVertex, weight);
                        update();
                        firstSelectedVertex = -1;
                        addingEdge = 0;
                    }
                }
            }
            return;
        }
        else if (event->button() == Qt::RightButton) {
            if(clickedVertex != -1){
                if(removingEdge == 1){
                    graph.removeEdge(firstSelectedVertex, clickedVertex);
                    firstSelectedVertex = -1;
                    removingEdge = 0;
                }
                else if(chosenAlgorithm == 1){
                    runAStar(firstSelectedVertex, clickedVertex);
                    firstSelectedVertex = -1;
                    chosenAlgorithm = -1;
                }
                else if(chosenAlgorithm == 2){
                    runDijkstra(firstSelectedVertex, clickedVertex);
                    firstSelectedVertex = -1;
                    chosenAlgorithm = -1;
                }
                else{
                    QMenu* contextMenu = new QMenu(this);

                    QAction *deleteEdge = contextMenu->addAction("Delete Edge");
                    QAction *runBFSAction = contextMenu->addAction("Run BFS");
                    QAction *runDFSAction = contextMenu->addAction("Run DFS");
                    QAction *runAstarAction = contextMenu->addAction("Run A*");
                    QAction *runDijkstraAction = contextMenu->addAction("Run Dijkstra");
                    QAction *selectedAction = contextMenu->exec(event->globalPosition().toPoint());
                    if(selectedAction == deleteEdge){
                        console.append("Right-click the second vertex to remove the edge.");
                        firstSelectedVertex = clickedVertex;
                        removingEdge = 1;
                    }
                    else if (selectedAction == runBFSAction) {
                        runBFS(clickedVertex);
                    }
                    else if (selectedAction == runDFSAction) {
                        runDFS(clickedVertex);
                    }
                    else if (selectedAction == runAstarAction) {
                        console.append("Right-click the second vertex to run the algorithm.");
                        firstSelectedVertex = clickedVertex;
                        chosenAlgorithm = 1;
                    }
                    else if (selectedAction == runDijkstraAction) {
                        console.append("Right-click the second vertex to run the algorithm.");
                        firstSelectedVertex = clickedVertex;
                        chosenAlgorithm = 2;
                    }
                }
            }
            return;
        }
        else if (event->button() == Qt::LeftButton) {
            if(clickedVertex != -1){
                grabbedVertex = clickedVertex;
                dragOffset = graph.getVertex(clickedVertex) - event->pos();
                setCursor(Qt::ClosedHandCursor);
                return;
            }
            else{
                graph.addVertex(event->pos());
                update();
            }
        }
        else if(event->button() == Qt::MiddleButton){
            if (clickedVertex != -1){
                graph.removeVertex(clickedVertex);
                update();
            }
        }
    }
    else{
        if (event->button() == Qt::LeftButton && clickedVertex != -1) {
            grabbedVertex = clickedVertex;

            dragOffset = graph.getVertex(clickedVertex) - event->pos();

            setCursor(Qt::ClosedHandCursor);
            return;

        }
    }
    update();
}

void GraphUserInterface::mouseMoveEvent(QMouseEvent *event){

    if (grabbedVertex != -1) {
        graph.changeVertexPosition(grabbedVertex, event->pos() + dragOffset);

        if(heuristicLabels.size() != 0){
            QLabel *label = heuristicLabels[grabbedVertex];

            label->move(graph.getVertex(grabbedVertex).x() - label->width()/2, graph.getVertex(grabbedVertex).y() - label->height()/2);
        }
        update();
    }
}

void GraphUserInterface::mouseReleaseEvent(QMouseEvent *event){
    Q_UNUSED(event);
    if (grabbedVertex != -1) {
        grabbedVertex = -1;
        setCursor(Qt::ArrowCursor);
    }
}

int GraphUserInterface::findClickedVertex(const QPoint &position) {
    for (int i = 0; i < graph.verticesSize(); i++) {
        QPoint diff = graph.getVertex(i) - position;

        if (diff.x() * diff.x() + diff.y() * diff.y() <= radius*radius) {
            return i;
        }
    }
    return -1;
}

void GraphUserInterface::updateConsole(QString message){
    console.append(message);
    update();
}

void GraphUserInterface::updateGraphInfoLabel(bool isEmpty, int numVertices, int numEdges) {
    QString newInfo = QString("Empty: %1\nNumber of vertices: %2\nNumber of edges: %3").arg(isEmpty ? "Yes" : "No").arg(numVertices).arg(numEdges);
    graphInfoLabel.setText(newInfo);
    update();
}

void GraphUserInterface::runBFS(int start) {
    visualizationRunning = 1;

    QVector<QPair<int, int>> bfsPath = graph.bfs(start);

    if(bfsPath.size() == 0){
        visualizationRunning = 0;
        console.append("Selected vertex has no neighbors — visualization cancelled.");
        return;
    }
    visitedVertices.insert(start);
    update();
    int step = 0;

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer, bfsPath, step]() mutable {
        if(visualizationRunning == 0){
            timer->deleteLater();
        }
        else{
            if(visualizationPaused) {
                return;
            }
            if (step < bfsPath.size()) {
                visitedVertices.insert(bfsPath[step].first);
                visitedEdges.insert(bfsPath[step]);
                visitedVertices.insert(bfsPath[step].second);
                step++;
            }
            else {
                timer->stop();
                visitedVertices.clear();
                visitedEdges.clear();
                visualizationRunning = 0;
                timer->deleteLater();
            }
            update();
        }
    });
    timer->start(500);
}

void GraphUserInterface::runDFS(int start) {
    visualizationRunning = 1;

    QVector<QPair<int, int>> dfsPath = graph.dfs(start);

    if(dfsPath.size() == 0){
        visualizationRunning = 0;
        console.append("Selected vertex has no neighbors — visualization cancelled.");
        return;
    }
    visitedVertices.insert(start);
    update();

    int step = 0;

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer, dfsPath, step]() mutable {
        if(visualizationRunning == 0){
            timer->deleteLater();
        }
        else{
            if(visualizationPaused) {
                return;
            }
            if (step < dfsPath.size()) {
                visitedVertices.insert(dfsPath[step].first);
                visitedEdges.insert(dfsPath[step]);
                visitedVertices.insert(dfsPath[step].second);
                step++;
            }
            else {
                timer->stop();
                visitedVertices.clear();
                visitedEdges.clear();
                visualizationRunning = 0;
                timer->deleteLater();
            }
            update();
        }

    });
    timer->start(500);
}
void GraphUserInterface::runDijkstra(int start, int end) {
    if(start == end){
        console.append("Start and end are the same — visualization cancelled.");
        return;
    }

    visualizationRunning = 1;

    QPair<QVector<QPair<int, int>>,QVector<QPair<int, int>>> dijkstraPaths = graph.dijkstra(start, end);

    if(dijkstraPaths.first.size() == 0){
        visualizationRunning = 0;
        console.append("Start vertex has no neighbors — visualization cancelled.");
        return;
    }

    int step = 0;

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer, dijkstraPaths, step]() mutable {

        if(visualizationRunning == 0){
            timer->deleteLater();
        }
        else{
            if(visualizationPaused) {
                return;
            }
            if (step < dijkstraPaths.first.size()) {
                QPair<int, int> edge = dijkstraPaths.first[step];
                visitedVertices.insert(edge.second);
                visitedVertices.insert(edge.first);
                visitedEdges.insert(edge);
                update();
                step++;
            } else {
                timer->stop();
                visitedVertices.clear();
                visitedEdges.clear();
                update();
                step = 0;
                if(dijkstraPaths.second.size() == 0){
                    console.append("No path to the goal found.");
                    visualizationRunning = 0;
                }
                else{
                    while(step < dijkstraPaths.second.size()){
                        QPair<int, int> edge = dijkstraPaths.second[step];
                        visitedVertices.insert(edge.second);
                        visitedEdges.insert(edge);
                        visitedVertices.insert(edge.first);
                        step++;
                    }
                    update();
                    QTimer::singleShot(4000, this, [this]() {
                        visitedVertices.clear();
                        visitedEdges.clear();
                        visualizationRunning = 0;
                        update();
                    });
                }
                timer->deleteLater();
            }
        }

    });
    timer->start(500);
}

void GraphUserInterface::runAStar(int start, int end) {
    if(start == end){
        console.append("Start and end are the same — visualization cancelled.");
        return;
    }

    visualizationRunning = 1;

    std::pair<QVector<QPair<int, int>>,QVector<QPair<int, int>>> aStarPaths = graph.aStar(start, end);

    if(aStarPaths.first.size() == 0){
        visualizationRunning = 0;
        console.append("Start vertex has no neighbors — visualization cancelled.");
        return;
    }

    int step = 0;
    for(int i = 0; i < graph.verticesSize(); i++){
        QLabel *label = new QLabel(this);
        label->setText(QString::number(20.0 * graph.euclideanDistance(i, end) / Graph::heuristicScaleDivisor(), 'f', 1));
        label->adjustSize();
        label->move(graph.getVertex(i).x() - label->width()/2, graph.getVertex(i).y() - label->height()/2);
        label->show();
        heuristicLabels.push_back(label);
    }
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer, aStarPaths, step]() mutable {
        if(visualizationRunning == 0){
            timer->deleteLater();
        }
        else{
            if(visualizationPaused) {
                return;
            }
            if (step < aStarPaths.first.size()) {
                QPair<int, int> edge = aStarPaths.first[step];
                visitedVertices.insert(edge.first);
                visitedVertices.insert(edge.second);
                visitedEdges.insert(edge);
                update();
                step++;
            } else {
                timer->stop();
                visitedVertices.clear();
                visitedEdges.clear();
                update();
                step = 0;
                if(aStarPaths.second.size() == 0){
                    console.append("No path to the goal found.");
                    for(QLabel* label : heuristicLabels) {
                        label->hide();
                        delete label;
                    }
                    heuristicLabels.clear();
                    visualizationRunning = 0;
                    update();
                }
                else{
                    while(step < aStarPaths.second.size()){
                        QPair<int, int> edge = aStarPaths.second[step];
                        visitedVertices.insert(edge.first);
                        visitedVertices.insert(edge.second);
                        visitedEdges.insert(edge);
                        step++;
                    }
                    QTimer::singleShot(4000, this, [this]() {
                        visitedVertices.clear();
                        visitedEdges.clear();
                        for(QLabel* label : heuristicLabels) {
                            label->hide();
                            delete label;
                        }
                        heuristicLabels.clear();
                        visualizationRunning = 0;
                        update();
                    });
                    update();
                }
                timer->deleteLater();
            }
        }

    });
    timer->start(500);
}


void GraphUserInterface::onLoadClicked() {
    visualizationRunning = 0;
    removingEdge = 0;
    addingEdge = 0;
    firstSelectedVertex = -1;
    chosenAlgorithm = -1;

    for(QLabel* label : heuristicLabels) {
        delete label;
    }
    heuristicLabels.clear();
    visitedEdges.clear();
    visitedVertices.clear();

    QString filename = QFileDialog::getOpenFileName(this, "Open File", "", "*.txt");
    if (!filename.isEmpty()) {
        graph.loadFromFile(filename.toStdString());
    }
}

void GraphUserInterface::onSaveClicked() {
    QString filename = QFileDialog::getSaveFileName(this, "Save File", "", "*.txt");
    if (!filename.isEmpty()) {
        graph.saveToFile(filename.toStdString());
    }
}

void GraphUserInterface::onClearClicked() {
    visualizationRunning = 0;
    removingEdge = 0;
    addingEdge = 0;
    firstSelectedVertex = -1;
    chosenAlgorithm = -1;

    for(QLabel* label : heuristicLabels) {
        delete label;
    }
    heuristicLabels.clear();

    visitedEdges.clear();
    visitedVertices.clear();

    graph.clear();
}

void GraphUserInterface::onAbortClicked() {
    for(QLabel* label : heuristicLabels) {
        delete label;
    }
    heuristicLabels.clear();
    visualizationRunning = 0;
    visitedEdges.clear();
    visitedVertices.clear();
    update();
}


