#include "PathAlgorithm.h"
#include "QtConcurrent/qtconcurrentrun.h"
#include <iostream>
#include <queue>
#include <map>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <QtConcurrent>
#include <QFuture>
#include <stack>
#include <list>
#include <QDebug> // Include for qDebug()
#include <QDateTime> // Include for QDateTime::currentMSecsSinceEpoch()
#include <QThread> // Include for QThread::currentThreadId()
void PathAlgorithm::performRecursiveBackTrackerAlgorithm(QPromise<int>& promise)
{
    qDebug() << "Maze: Algorithm started in worker thread:" << QThread::currentThreadId();
    promise.suspendIfRequested();
    if (promise.isCanceled()) {
        emit pathfindingSearchCompleted(0, 0); // Emit with 0,0 on cancel for maze
        return;
    }

    // Initialize all nodes as obstacles (except start/end)
    for (int index = 0; index < widthGrid * heightGrid; index++){
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            qDebug() << "Maze: Algorithm cancelled during init.";
            emit pathfindingSearchCompleted(0, 0); // Emit with 0,0 on cancel for maze
            return;
        }

        // Only set to obstacle if it's not the start or end node
        if (index != gridNodes.startIndex && index != gridNodes.endIndex){
            gridNodes.Nodes[index].obstacle = true;
            emit updatedScatterGridView(FREETOOBSTACLE, index);
        }
    }

    std::stack<Node*> stackUnVisitedNodes;
    // Choose a random starting cell for maze generation
    int randomIndex = rand() % (widthGrid * heightGrid);
    Node* startMazeNode = &(gridNodes.Nodes[randomIndex]);

    stackUnVisitedNodes.push(startMazeNode);
    startMazeNode->visited = true;
    // Ensure the start node for maze generation is not an obstacle
    if (startMazeNode->obstacle) {
        startMazeNode->obstacle = false;
        emit updatedScatterGridView(OBSTACLETOFREE, randomIndex);
    }

    int numberVisitedCells = 1;
    int offset = 2; // For carving paths in a grid, typically move 2 steps
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Initial sleep for maze generation

    while(numberVisitedCells < widthGrid * heightGrid && !stackUnVisitedNodes.empty() ){
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            qDebug() << "Maze: Algorithm cancelled during loop.";
            emit pathfindingSearchCompleted(0, 0); // Emit with 0,0 on cancel for maze
            return;
        }

        Node* currentNode = stackUnVisitedNodes.top();
        //int currentIndex = coordToIndex(currentNode->xCoord, currentNode->yCoord, widthGrid);
        std::vector<int> availableNeighboursDirections; // 0:East, 1:South, 2:West, 3:North

        // Check potential neighbors (2 steps away for carving)
        // East
        int eastNeighborX = currentNode->xCoord + offset;
        int eastPathX = currentNode->xCoord + offset - 1;
        if (eastNeighborX <= widthGrid && !gridNodes.Nodes[coordToIndex(eastNeighborX, currentNode->yCoord, widthGrid)].visited){
            availableNeighboursDirections.push_back(0);
        }
        // South
        int southNeighborY = currentNode->yCoord - offset;
        int southPathY = currentNode->yCoord - offset + 1;
        if (southNeighborY >= 1 && !gridNodes.Nodes[coordToIndex(currentNode->xCoord, southNeighborY, widthGrid)].visited){
            availableNeighboursDirections.push_back(1);
        }
        // West
        int westNeighborX = currentNode->xCoord - offset;
        int westPathX = currentNode->xCoord - offset + 1;
        if (westNeighborX >= 1 && !gridNodes.Nodes[coordToIndex(westNeighborX, currentNode->yCoord, widthGrid)].visited){
            availableNeighboursDirections.push_back(2);
        }
        // North
        int northNeighborY = currentNode->yCoord + offset;
        int northPathY = currentNode->yCoord + offset - 1;
        if (northNeighborY <= heightGrid && !gridNodes.Nodes[coordToIndex(currentNode->xCoord, northNeighborY, widthGrid)].visited){
            availableNeighboursDirections.push_back(3);
        }


        if (!(availableNeighboursDirections.empty())){
            int randomDirection = availableNeighboursDirections[rand() % availableNeighboursDirections.size()];
            Node* nextNode = nullptr;
            int pathIndexToClear = -1;

            switch (randomDirection) {
            case 0: // East
                nextNode = &(gridNodes.Nodes[coordToIndex(eastNeighborX, currentNode->yCoord, widthGrid)]);
                pathIndexToClear = coordToIndex(eastPathX, currentNode->yCoord, widthGrid);
                break;
            case 1: // South
                nextNode = &(gridNodes.Nodes[coordToIndex(currentNode->xCoord, southNeighborY, widthGrid)]);
                pathIndexToClear = coordToIndex(currentNode->xCoord, southPathY, widthGrid);
                break;
            case 2: // West
                nextNode = &(gridNodes.Nodes[coordToIndex(westNeighborX, currentNode->yCoord, widthGrid)]);
                pathIndexToClear = coordToIndex(westPathX, currentNode->yCoord, widthGrid);
                break;
            case 3: // North
                nextNode = &(gridNodes.Nodes[coordToIndex(currentNode->xCoord, northNeighborY, widthGrid)]);
                pathIndexToClear = coordToIndex(currentNode->xCoord, northPathY, widthGrid);
                break;
            default:
                break;
            }

            if (nextNode) {
                // Carve path (make it free)
                if (pathIndexToClear != -1) {
                    gridNodes.Nodes[pathIndexToClear].obstacle = false;
                    emit updatedScatterGridView(OBSTACLETOFREE, pathIndexToClear);
                }
                // Make next node free and push to stack
                nextNode->obstacle = false;
                nextNode->visited = true;
                emit updatedScatterGridView(OBSTACLETOFREE, coordToIndex(nextNode->xCoord, nextNode->yCoord, widthGrid));
                stackUnVisitedNodes.push(nextNode);
                numberVisitedCells++;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(speedVisualization));
        }else{
            // Backtrack
            stackUnVisitedNodes.pop();
        }
    }

    // Reset visited flags for next pathfinding run
    for(Node& node: gridNodes.Nodes) {
        node.visited = false;
        node.nextUp = false;
    }

    emit algorithmCompleted();
    // For maze generation, nodesVisited and pathLength are typically 0 or not applicable
    emit pathfindingSearchCompleted(0,0);
    qDebug() << "Maze: Algorithm completed. Emitting algorithmCompleted() and pathfindingSearchCompleted()";
}
//maze generation using prims algorithm
void PathAlgorithm::performPrimsMazeAlgorithm(QPromise<int>& promise)
{
    qDebug() << "Maze: Prim's Algorithm started in worker thread:" << QThread::currentThreadId();
    promise.suspendIfRequested();
    if (promise.isCanceled()) {
        emit pathfindingSearchCompleted(0, 0);
        return;
    }

    // Initialize all nodes as obstacles (except start/end)
    for (int index = 0; index < widthGrid * heightGrid; index++) {
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            qDebug() << "Maze: Algorithm cancelled during init.";
            emit pathfindingSearchCompleted(0, 0);
            return;
        }

        if (index != gridNodes.startIndex && index != gridNodes.endIndex) {
            gridNodes.Nodes[index].obstacle = true;
            emit updatedScatterGridView(FREETOOBSTACLE, index);
        }
    }

    // Choose a random starting cell for maze generation
    int randomIndex = rand() % (widthGrid * heightGrid);
    Node* startNode = &(gridNodes.Nodes[randomIndex]);

    // Ensure the start node is not an obstacle
    if (startNode->obstacle) {
        startNode->obstacle = false;
        emit updatedScatterGridView(OBSTACLETOFREE, randomIndex);
    }

    // List of frontier cells (edges between maze and walls)
    std::vector<Node*> frontier;
    addFrontierCells(startNode, frontier);

    int numberVisitedCells = 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    while (!frontier.empty()) {
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            qDebug() << "Maze: Algorithm cancelled during loop.";
            emit pathfindingSearchCompleted(0, 0);
            return;
        }

        // Select a random frontier cell
        int randomFrontierIndex = rand() % frontier.size();
        Node* frontierNode = frontier[randomFrontierIndex];
        frontier.erase(frontier.begin() + randomFrontierIndex);

        // Get all neighboring maze cells (2 steps away)
        std::vector<Node*> mazeNeighbors = getMazeNeighbors(frontierNode);

        if (!mazeNeighbors.empty()) {
            // Connect the frontier cell to a random maze neighbor
            Node* mazeNeighbor = mazeNeighbors[rand() % mazeNeighbors.size()];
            connectNodes(frontierNode, mazeNeighbor);

            // Add new frontier cells
            addFrontierCells(frontierNode, frontier);
            numberVisitedCells++;

            std::this_thread::sleep_for(std::chrono::milliseconds(speedVisualization));
        }
    }

    // Reset visited flags for next pathfinding run
    for (Node& node : gridNodes.Nodes) {
        node.visited = false;
        node.nextUp = false;
    }

    emit algorithmCompleted();
    emit pathfindingSearchCompleted(0, 0);
    qDebug() << "Maze: Prim's Algorithm completed.";
}

// Helper function to add frontier cells (unvisited neighbors 2 steps away)
void PathAlgorithm::addFrontierCells(Node* node, std::vector<Node*>& frontier) {
    const int offsets[4][2] = {{2, 0}, {-2, 0}, {0, 2}, {0, -2}}; // E, W, N, S

    for (const auto& offset : offsets) {
        int newX = node->xCoord + offset[0];
        int newY = node->yCoord + offset[1];

        if (newX >= 1 && newX <= widthGrid && newY >= 1 && newY <= heightGrid) {
            int index = coordToIndex(newX, newY, widthGrid);
            Node* neighbor = &(gridNodes.Nodes[index]);

            if (neighbor->obstacle && !neighbor->visited) {
                neighbor->visited = true; // Mark as frontier
                frontier.push_back(neighbor);
            }
        }
    }
}

// Helper function to get maze neighbors (visited cells 2 steps away)
std::vector<Node*> PathAlgorithm::getMazeNeighbors(Node* node) {
    std::vector<Node*> mazeNeighbors;
    const int offsets[4][2] = {{2, 0}, {-2, 0}, {0, 2}, {0, -2}}; // E, W, N, S

    for (const auto& offset : offsets) {
        int newX = node->xCoord + offset[0];
        int newY = node->yCoord + offset[1];

        if (newX >= 1 && newX <= widthGrid && newY >= 1 && newY <= heightGrid) {
            int index = coordToIndex(newX, newY, widthGrid);
            Node* neighbor = &(gridNodes.Nodes[index]);

            if (!neighbor->obstacle) { // Part of the maze
                mazeNeighbors.push_back(neighbor);
            }
        }
    }

    return mazeNeighbors;
}

// Helper function to connect two nodes by clearing the path between them
void PathAlgorithm::connectNodes(Node* a, Node* b) {
    // Calculate midpoint
    int midX = (a->xCoord + b->xCoord) / 2;
    int midY = (a->yCoord + b->yCoord) / 2;

    // Clear both nodes and the path between them
    a->obstacle = false;
    b->obstacle = false;
    int midIndex = coordToIndex(midX, midY, widthGrid);
    gridNodes.Nodes[midIndex].obstacle = false;

    // Emit signals for visualization
    emit updatedScatterGridView(OBSTACLETOFREE, coordToIndex(a->xCoord, a->yCoord, widthGrid));
    emit updatedScatterGridView(OBSTACLETOFREE, midIndex);
    emit updatedScatterGridView(OBSTACLETOFREE, coordToIndex(b->xCoord, b->yCoord, widthGrid));
}

//maze generation using kruskal algorithm
void PathAlgorithm::performKruskalsMazeAlgorithm(QPromise<int>& promise)
{
    qDebug() << "Maze (Kruskal's): Algorithm started in worker thread:" << QThread::currentThreadId();

    // Cancel check
    promise.suspendIfRequested();
    if (promise.isCanceled()) {
        emit pathfindingSearchCompleted(0, 0);
        return;
    }

    // Step 1: Initialize all nodes as obstacles except start/end
    for (int index = 0; index < widthGrid * heightGrid; ++index) {
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            emit pathfindingSearchCompleted(0, 0);
            return;
        }

        if (index != gridNodes.startIndex && index != gridNodes.endIndex) {
            gridNodes.Nodes[index].obstacle = true;
            emit updatedScatterGridView(FREETOOBSTACLE, index);
        }
    }

    // Step 2: Initialize Disjoint Set Union (DSU)
    std::vector<int> parent(widthGrid * heightGrid);
    std::vector<int> rank(widthGrid * heightGrid, 0);

    for (int i = 0; i < widthGrid * heightGrid; ++i)
        parent[i] = i; // every cell initially its own parent




    const std::vector<QPair<int, int>> directions = {{2, 0}, {0, 2}}; // right and down
    std::vector<Wall> walls;
    for (int y = 1; y <=heightGrid ; y += 2) {
        for (int x = 1; x <= widthGrid; x += 2) {
            int cellIndex = coordToIndex(x, y, widthGrid);
            for (auto dir : directions) {
                int nx = x + dir.first;
                int ny = y + dir.second;
                if (nx <=widthGrid && ny <=heightGrid) {
                    int neighborIndex = coordToIndex(nx, ny, widthGrid);
                    int wallX = x + dir.first / 2;
                    int wallY = y + dir.second / 2;
                    int wallIndex = coordToIndex(wallX, wallY, widthGrid);
                    walls.push_back({wallIndex, cellIndex, neighborIndex});
                }
            }
        }
    }

    //Shuffle walls
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(walls.begin(), walls.end(), g);

    //Carve paths by removing walls
    for (const Wall& wall : walls) {
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            emit pathfindingSearchCompleted(0, 0);
            return;
        }

        int set1 = findSet(wall.cell1Index,parent);
        int set2 = findSet(wall.cell2Index,parent);

        if (set1 != set2) {
            unionSet(set1, set2,parent,rank);

            // Carve wall
            gridNodes.Nodes[wall.wallIndex].obstacle = false;
            emit updatedScatterGridView(OBSTACLETOFREE, wall.wallIndex);

            // Carve both cells
            gridNodes.Nodes[wall.cell1Index].obstacle = false;
            emit updatedScatterGridView(OBSTACLETOFREE, wall.cell1Index);

            gridNodes.Nodes[wall.cell2Index].obstacle = false;
            emit updatedScatterGridView(OBSTACLETOFREE, wall.cell2Index);

            std::this_thread::sleep_for(std::chrono::milliseconds(speedVisualization));
        }
    }

    //Reset visited/nextUp flags
    for (Node& node : gridNodes.Nodes) {
        node.visited = false;
        node.nextUp = false;
    }

    emit algorithmCompleted();
    emit pathfindingSearchCompleted(0, 0);
    qDebug() << "Maze (Kruskal's): Algorithm completed.";
}
int findSet(int x,std::vector<int>& parent)
{
    if(parent[x]==x) return x;
    return parent[x]=findSet(parent[x],parent);
}

bool unionSet(int a ,int b,std::vector<int> &parent,std::vector<int>& rank)
{
    a=findSet(a,parent);
    b=findSet(b,parent);
    if (a != b) {
        if (rank[a] < rank[b])
            parent[a] = b;
        else if (rank[b] < rank[a])
            parent[b] = a;
        else {
            parent[b] = a;
            rank[a]++;
        }
        return true;
    }
    return false;

}
void PathAlgorithm::performWilsonsAlgorithm(QPromise<int>& promise)
{
    qDebug() << "Maze: Wilson's Algorithm started in worker thread:" << QThread::currentThreadId();
    promise.suspendIfRequested();
    if (promise.isCanceled()) {
        emit pathfindingSearchCompleted(0, 0);
        return;
    }

    // Initialize all nodes as obstacles
    for (int index = 0; index < widthGrid * heightGrid; index++) {
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            qDebug() << "Maze: Algorithm cancelled during init.";
            emit pathfindingSearchCompleted(0, 0);
            return;
        }

        gridNodes.Nodes[index].obstacle = true;
        gridNodes.Nodes[index].visited = false;
        emit updatedScatterGridView(FREETOOBSTACLE, index);
    }

    // Create a set of unvisited nodes (only odd coordinates for proper maze generation)
    std::set<int> unvisited;
    for (int y = 1; y <= heightGrid; y += 2) {
        for (int x = 1; x <= widthGrid; x += 2) {
            unvisited.insert(coordToIndex(x, y, widthGrid));
        }
    }

    // Choose random starting cell
    auto it = unvisited.begin();
    std::advance(it, rand() % unvisited.size());
    int initialIndex = *it;
    unvisited.erase(it);

    gridNodes.Nodes[initialIndex].obstacle = false;
    gridNodes.Nodes[initialIndex].visited = true;
    emit updatedScatterGridView(OBSTACLETOFREE, initialIndex);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    while (!unvisited.empty()) {
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            qDebug() << "Maze: Algorithm cancelled during loop.";
            emit pathfindingSearchCompleted(0, 0);
            return;
        }

        // Pick random unvisited cell
        it = unvisited.begin();
        std::advance(it, rand() % unvisited.size());
        int startIndex = *it;
        Node* current = &gridNodes.Nodes[startIndex];

        // Perform loop-erased random walk
        std::map<int, int> parent; // child -> parent mapping
        std::set<int> walkVisited;
        parent[startIndex] = startIndex;
        walkVisited.insert(startIndex);

        while (unvisited.count(coordToIndex(current->xCoord, current->yCoord, widthGrid))) {
            std::vector<std::pair<int, int>> directions;
            int x = current->xCoord;
            int y = current->yCoord;

            if (x + 2 <= widthGrid) directions.emplace_back(2, 0);
            if (x - 2 >= 1) directions.emplace_back(-2, 0);
            if (y + 2 <= heightGrid) directions.emplace_back(0, 2);
            if (y - 2 >= 1) directions.emplace_back(0, -2);

            if (directions.empty()) break;

            auto [dx, dy] = directions[rand() % directions.size()];
            int nextX = x + dx;
            int nextY = y + dy;
            int nextIndex = coordToIndex(nextX, nextY, widthGrid);

            if (walkVisited.count(nextIndex)) {
                int loopStart = nextIndex;
                int currentIndex = coordToIndex(current->xCoord, current->yCoord, widthGrid);
                while (currentIndex != loopStart) {
                    walkVisited.erase(currentIndex);
                    currentIndex = parent[currentIndex];
                    current = &gridNodes.Nodes[currentIndex];
                }
            } else {
                parent[nextIndex] = coordToIndex(current->xCoord, current->yCoord, widthGrid);
                walkVisited.insert(nextIndex);
                current = &gridNodes.Nodes[nextIndex];
            }

            if (!gridNodes.Nodes[nextIndex].obstacle) {
                break;
            }
        }

        // Carve the path
        std::vector<int> pathIndices;
        int pathIndex = coordToIndex(current->xCoord, current->yCoord, widthGrid);
        while (parent[pathIndex] != pathIndex) {
            pathIndices.push_back(pathIndex);
            pathIndex = parent[pathIndex];
        }
        pathIndices.push_back(pathIndex);

        for (size_t i = pathIndices.size() - 1; i > 0; i--) {
            int currentIdx = pathIndices[i];
            int nextIdx = pathIndices[i - 1];

            Node& currentNode = gridNodes.Nodes[currentIdx];
            Node& nextNode = gridNodes.Nodes[nextIdx];

            int wallX = (currentNode.xCoord + nextNode.xCoord) / 2;
            int wallY = (currentNode.yCoord + nextNode.yCoord) / 2;
            int wallIdx = coordToIndex(wallX, wallY, widthGrid);

            currentNode.obstacle = false;
            nextNode.obstacle = false;
            gridNodes.Nodes[wallIdx].obstacle = false;

            unvisited.erase(currentIdx);
            unvisited.erase(nextIdx);

            emit updatedScatterGridView(OBSTACLETOFREE, currentIdx);
            emit updatedScatterGridView(OBSTACLETOFREE, nextIdx);
            emit updatedScatterGridView(OBSTACLETOFREE, wallIdx);

            std::this_thread::sleep_for(std::chrono::milliseconds(speedVisualization));
        }
    }

    // Reset visited flags (nextUp is not used here)
    for (Node& node : gridNodes.Nodes) {
        node.visited = false;
        node.nextUp = false;
    }

    emit algorithmCompleted();
    emit pathfindingSearchCompleted(0, 0);
    qDebug() << "Maze: Wilson's Algorithm completed (maze only — no pathfinding)";
}
