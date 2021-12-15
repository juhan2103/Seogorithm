from collections import defaultdict


def dfs(here, parent):
    global cnt
    cnt = cnt + 1
    order[here] = cnt
    ret = order[here]

    for next in graph[here]:
        if next == parent:  # 역으로 가는 것은 무시
            continue
        if order[next]:  # 방문한 적이 있으면 continue
            ret = min(ret, order[next])
            continue
        subtree = dfs(next, here)
        ret = min(subtree, ret)

        if(subtree > order[here]):
            cutEdge.add(tuple(sorted([here, next])))
    return ret


V = int(input("정점개수를 입력 >> "))
E = int(input("간선의 개수를 입력하세요 >> "))
graph = defaultdict(set)
cutEdge = set()
candidates = set()

for _ in range(E):
    a, b = map(int, input("간선의 양 끝점을 입력하고 엔터를 누르시오(정점은 1부터 시작) >> ").split())
    graph[a].add(b)
    graph[b].add(a)
    candidates.add(a)
    candidates.add(b)

order = [None] * (V+1)  # 순회 체크 리스트
cnt = 0
dfs(1, None)
for a, b in cutEdge:
    print("간선(", a, ",", b, ")는 브릿지입니다.")
