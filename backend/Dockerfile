FROM node:20-slim

RUN apt-get update && apt-get install -y --no-install-recommends g++ && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY package.json package-lock.json* ./
RUN npm install --omit=dev

COPY . .
RUN npm run build:cpp

ENV PORT=3001
EXPOSE 3001
CMD ["npm", "start"]