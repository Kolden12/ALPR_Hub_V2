import uvicorn
from fastapi import FastAPI

app = FastAPI(title="Sabre ALPR Hub API")

@app.get("/health")
async def health_check():
    return {"status": "online"}

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
