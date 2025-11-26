FROM node:22

WORKDIR /FullApp

COPY . .

RUN npm install

ENV PORT=3000

EXPOSE 3000

CMD ["npm", "start"]