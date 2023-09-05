FROM node:alpine AS builder
WORKDIR /app
COPY . .
RUN npm install
RUN npm run clean
RUN npm run build

FROM node:slim
WORKDIR /app
COPY --from=builder ./app/dist ./dist
COPY package* ./
RUN npm install --production
CMD npm start
