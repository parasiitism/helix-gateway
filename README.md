# Helix Gateway

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-Data%20Plane-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++" />
  <img src="https://img.shields.io/badge/LLM-Gateway-7C3AED?style=for-the-badge" alt="LLM Gateway" />
  <img src="https://img.shields.io/badge/Observability-OpenTelemetry-16A34A?style=for-the-badge" alt="OpenTelemetry" />
  <img src="https://img.shields.io/badge/Performance-Sub%20Millisecond-F97316?style=for-the-badge" alt="Performance" />
</p>

Helix Gateway is a high-performance LLM traffic gateway with a C++ data plane, Python control plane, semantic caching, cost-aware routing, and OpenTelemetry GenAI tracing. The project is organized like an infrastructure system: fast request path, policy APIs, benchmark workloads, deployment artifacts, and observability support.

## What It Demonstrates

- Low-latency C++ gateway design for LLM traffic.
- Control-plane APIs for tenants, routes, budgets, prompts, evals, and virtual keys.
- Semantic cache and routing concepts for model traffic optimization.
- Sidecar services for embeddings and guardrails.
- Benchmark tools for synthetic and replay-based workloads.
- Docker Compose deployment surfaces for local infrastructure.
- OpenTelemetry-oriented tracing and analytics structure.

## Architecture

```mermaid
flowchart LR
    A[Client Request] --> B[C++ Data Plane]
    B --> C{Routing Policy}
    C --> D[Model Provider]
    C --> E[Semantic Cache]
    B --> F[OpenTelemetry Traces]
    G[Python Control Plane] --> C
    G --> H[Analytics and Budgets]
    I[Sidecars] --> B
    classDef client fill:#dbeafe,stroke:#2563eb,color:#1e3a8a,stroke-width:2px
    classDef data fill:#fee2e2,stroke:#dc2626,color:#7f1d1d,stroke-width:2px
    classDef control fill:#ede9fe,stroke:#7c3aed,color:#4c1d95,stroke-width:2px
    classDef obs fill:#dcfce7,stroke:#16a34a,color:#14532d,stroke-width:2px
    class A client
    class B,C,D,E,I data
    class G,H control
    class F obs
```

## Repository Map

```text
data-plane/      C++ gateway implementation
control-plane/   Python APIs and control-plane services
benchmarks/      Mock OpenAI server and workload generators
deploy/          Docker Compose deployment examples
sidecars/        Embedder and guardrail sidecars
observability/   Telemetry and monitoring assets
docs/            Design and operational notes
```

## Local Development

```powershell
cd control-plane
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -e .
```

Deployment assets:

```powershell
cd ..\deploy
docker compose up --build
```

## Revision Notes

- Keep the data plane small and fast.
- Push configuration, analytics, and policy management into the control plane.
- Treat observability as a product feature for LLM infrastructure.
- Cache hits reduce cost and latency, but routing policy must protect correctness.

## Interview Talking Points

```text
The key design split is data plane versus control plane. The data plane should make
fast request decisions, while the control plane manages tenants, budgets, routing,
and analytics. This keeps the hot path lean while still supporting operational needs.
```
