#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define VERTEX_NUM 10 // 정점의 개수
#define RESULT_SIZE 100 // 결과 배열 크기

int check[VERTEX_NUM];
int count = 0;
int r; // 최대 vertex
int mang_num = 0; // 무선망 수
char* finalresult[RESULT_SIZE]; // 최종 무선망 집합 저장
char* DFSresult[RESULT_SIZE]; // DFS를 통해 찾은 집합 저장

//구조체 선언
typedef struct Graph {
	int n; // 정점의 갯수
	V_info* connected_V[VERTEX_NUM]; // 연결된 정점에 대한 정보
}Graph;

typedef struct V_info { // 정점에 대한 정보
	int v;
	struct V_info* link;
	int x; //x 좌표
	int y; //y 좌표
	int z; //z 좌표
}V_info;

// 함수 선언
Graph* init_Graph(void);
void init_check();
void add_V(Graph* graph);
void add_E(Graph* graph, int u, int v);
void DFS(Graph *graph, char** DFSresult);
void Tree(Graph *graph, int n, char** buffer);

int main() {
    srand(time(NULL)); // 난수 생성을 위한 초기 설정

    printf("무선망 길이 입력 (0 ~ 10) : ");
	scanf_s("%d", &r);
    
    // sum 동적할당, '.'으로 초기화, result 동적할당
	for (int i = 0; i < RESULT_SIZE; i++) {
		DFSresult[i] = (char*)malloc(sizeof(char) * 50);
		sprintf(DFSresult[i], "%s", ".");
		finalresult[i] = (char*)malloc(sizeof(char) * 50);
	}

    // Graph 초기화
    Graph* graph = init_Graph();

    // check 초기화 
    init_check(); 

	// 정점 랜덤 추가
	for (int i = 0; i < VERTEX_NUM; i++) {
		add_V(graph);
	}
	
	// 정점 사이의 길이 r 보다 작은 edge만 edge를 생성.
	for (int i = 0; i < VERTEX_NUM-1; i++) {
		for (int j = i+1; j < VERTEX_NUM; j++) {
			if (i == j) {
				// 자기자신 제외
			}
            else if ((sqrt(pow(graph->connected_V[i]->x - graph->connected_V[j]->x, 2) 
            + pow(graph->connected_V[i]->y - graph->connected_V[j]->y, 2) 
            + pow(graph->connected_V[i]->z - graph->connected_V[j]->z, 2)))<=(double)r) {
				// 무선망 길이보다 가까운 vertex만 연결
				add_E(graph, i, j);
			}
		}
	}
	
	// DFS로 정점의 집합 찾기
	DFS(graph, DFSresult);

	// 정점이 있는 집합만 result에 넣는다.
	for (int i = 0; i < count; i++) {
		if (strcmp(DFSresult[i],".")) {
			sprintf(finalresult[mang_num], "%s", DFSresult[i]);
			mang_num++;
		}
	}

	// 결과 출력
	printf("-------정점 V의 좌표-------\n");
	for (int i = 0; i < 10; i++) {
		printf("|V%d|x : %2d|y : %2d|z : %2d|\n",i,graph->connected_V[i]->x, graph->connected_V[i]->y, graph->connected_V[i]->z);
	}
    printf("\n");

	printf("정점의 개수 : %d\n최대 정점 사이의 거리 : %d\n", VERTEX_NUM, r);

	if (mang_num == 1) {
		printf("전체는 하나의 무선망으로 연결된다.\n");
	}
	else {
		printf("전체는 하나의 무선망으로 연결되지 않는다.\n");
		printf("무선망의 수 : %d개\n", mang_num);
	}
	printf("\n-------정점의 집합-------\n");

	for (int i = 0; i < mang_num; i++) {
		printf("%d번째 집합 - {%s}\n",i+1,finalresult[i]);
	}

	system("pause > nul"); // 난수 설정 pause

	return 0;
}

Graph* init_Graph(void) {
	Graph* graph = (Graph*)malloc(sizeof(Graph));
	graph->n = 0;
	for (int i = 0; i < VERTEX_NUM; i++)
		graph->connected_V[i] = NULL;
	return graph;
}

void init_check(){
    for (int i = 0; i < VERTEX_NUM; i++) {
		check[i] = 0;
	}
}

void add_V(Graph* graph) {
	int a = -6, b = 6; // 랜덤값 범위 : a~b
	graph->connected_V[graph->n] = (V_info*)malloc(sizeof(V_info));
	graph->connected_V[graph->n]->v = -1;
	graph->connected_V[graph->n]->link = NULL;
	graph->connected_V[graph->n]->x = rand() % (b - a + 1) + a; // x값 설정
	graph->connected_V[graph->n]->y = rand() % (b - a + 1) + a; // y값 설정
	graph->connected_V[graph->n]->z = rand() % (b - a + 1) + a; // z값 설정
	graph->n++;
}

void add_E(Graph* graph, int u, int v) {
	if (u >= graph->n || v >= graph->n) {
		printf("Graph 범위가 아닙니다.\n");
	}
	else {
		V_info* V_u = (V_info*)malloc(sizeof(V_info));
		V_info* V_v = (V_info*)malloc(sizeof(V_info));
		V_u->v = u;
		V_v->v = v;
		V_v->link = graph->connected_V[u]->link;
		graph->connected_V[u]->link = V_v;
		V_u->link = graph->connected_V[v]->link;
		graph->connected_V[v]->link = V_u;
	}
}


void DFS(Graph *graph, char** DFSresult) {
	for (int i = 0; i < VERTEX_NUM; i++) {
		Tree(graph, i, DFSresult);
		count++;
	}
}

void Tree(Graph *graph, int n, char** buffer) {
	if (check[n] == 0) {
		V_info* temp1;
		V_info* temp2;
		temp1 = graph->connected_V[n]->link;

		if (!strcmp(*(buffer + count), ".")) {
			sprintf(*(buffer + count), "%d", n);
		}
		else {
			sprintf(*(buffer + count), "%s %d", *(buffer + count), n);
		}
		check[n] = 1;
		while (temp1 != NULL) {
			temp2 = temp1->link;
			if (check[temp1->v] == 0)
				Tree(graph, temp1->v, buffer);
			temp1 = temp2;
		}
	}
}