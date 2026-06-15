#!/bin/bash
# Login
TOKEN=$(curl -s -X POST http://localhost:8080/api/auth/login -d '{"username":"admin","password":"admin"}' -H "Content-Type: application/json" | grep -o '"token":"[^"]*' | grep -o '[^"]*$')
echo "Token: $TOKEN"

# Connect camera (mock id 0)
echo "Connecting camera..."
curl -s -X POST http://localhost:8080/api/cameras/select -H "Authorization: Bearer $TOKEN" -d '{"id":0}' -H "Content-Type: application/json"

# Disconnect camera
echo -e "\nDisconnecting camera..."
curl -s -X POST http://localhost:8080/api/cameras/disconnect -H "Authorization: Bearer $TOKEN"

echo -e "\nDone."
