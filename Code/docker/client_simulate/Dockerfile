FROM python
COPY . /app
WORKDIR /app
RUN python -m pip install -r requirements.txt
CMD ["python", "client_simulation/main.py"]