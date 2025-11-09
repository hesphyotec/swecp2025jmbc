import pandas as pd
from sklearn.feature_extraction.text import TfidfVectorizer
import numpy as np
from sklearn.metrics.pairwise import cosine_similarity
from collections import defaultdict
import sqlite3

# corpus = ""

# with open("output.txt", 'r', encoding='utf-8') as f:
#         corpus = f.read()

# tfidf_vec = TfidfVectorizer()
# tfidf_matrix = tfidf_vec.fit_transform([corpus])
# df = pd.DataFrame(tfidf_matrix.toarray(), columns =tfidf_vec.get_feature_names_out())

# df.to_excel("tfidf.xlsx")

con = sqlite3.connect('core.db')
cursor = con.cursor()
ins = pd.read_excel("tfidf.xlsx")
#cursor.execute(f"ALTER TABLE Ingredients ADD COLUMN tfidf DOUBLE;")

for values in ins.columns.values:
    temp = values
    search = values.replace("_", " ")
    cursor.execute(f"SELECT EXISTS(SELECT 1 FROM Ingredients WHERE name = '{search}' COLLATE NOCASE);")
    if(cursor.fetchone()[0] == 1):
        double = ins[temp][0]
        cursor.execute(f"UPDATE Ingredients SET tfidf = {double} WHERE name = '{search}' COLLATE NOCASE;")
        print("Entered", search)

con.commit()
con.close()